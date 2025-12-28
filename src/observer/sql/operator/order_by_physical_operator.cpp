/*
 * OrderBy physical operator implementation
 */

#include "sql/operator/order_by_physical_operator.h"
#include "common/log/log.h"

using namespace std;

OrderByPhysicalOperator::OrderByPhysicalOperator(vector<unique_ptr<Expression>> &&order_by_exprs)
  : cursor_(0), cur_tuple_(nullptr)
{
  order_by_items_.reserve(order_by_exprs.size());
  for (auto &e : order_by_exprs) {
    OrderByItem item;
    item.expr = std::move(e);
    item.asc = true;
    order_by_items_.push_back(std::move(item));
  }
}

OrderByPhysicalOperator::OrderByPhysicalOperator(vector<OrderByItem> &&order_by_items)
  : order_by_items_(std::move(order_by_items)), cursor_(0), cur_tuple_(nullptr)
{
}

RC OrderByPhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  PhysicalOperator *child = children_[0].get();
  RC rc = child->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  // materialize all rows and compute sort keys using the original child tuple
  while (true) {
    rc = child->next();
    if (rc == RC::RECORD_EOF) {
      break;
    }
    if (rc != RC::SUCCESS) {
      LOG_WARN("child next failed: %s", strrc(rc));
      return rc;
    }

    Tuple *t = child->current_tuple();
    RowItem item;
    if (OB_FAIL(ValueListTuple::make(*t, item.row))) {
      LOG_WARN("failed to materialize tuple");
      return RC::INTERNAL;
    }

    // compute sort keys from the original tuple (so FieldExpr can lookup by table/field)
    for (auto &it : order_by_items_) {
      Value key;
      if (it.expr) {
        RC krc = it.expr->get_value(*t, key);
        if (krc != RC::SUCCESS) {
          key.reset();
        }
      } else {
        key.reset();
      }
      item.keys.push_back(key);
    }

    rows_.push_back(std::move(item));
  }

  // sort rows by precomputed keys
  if (!order_by_items_.empty()) {
    sort(rows_.begin(), rows_.end(), [&](const RowItem &a, const RowItem &b) {
      for (size_t i = 0; i < a.keys.size() && i < b.keys.size() && i < order_by_items_.size(); ++i) {
        int cmp = a.keys[i].compare(b.keys[i]);
        if (cmp == 0) {
          continue;
        }
        // respect ASC/DESC flag
        if (order_by_items_[i].asc) {
          return cmp < 0;
        } else {
          return cmp > 0;
        }
      }
      return false;
    });
  }

  cursor_ = 0;
  if (!rows_.empty()) {
    cur_tuple_ = &rows_[0].row;
  }
  return RC::SUCCESS;
}

RC OrderByPhysicalOperator::next()
{
  if (cursor_ >= rows_.size()) {
    return RC::RECORD_EOF;
  }
  cur_tuple_ = &rows_[cursor_++].row;
  return RC::SUCCESS;
}

RC OrderByPhysicalOperator::close()
{
  if (!children_.empty()) {
    children_[0]->close();
  }
  rows_.clear();
  cur_tuple_ = nullptr;
  cursor_ = 0;
  return RC::SUCCESS;
}

Tuple *OrderByPhysicalOperator::current_tuple()
{
  return cur_tuple_;
}

RC OrderByPhysicalOperator::tuple_schema(TupleSchema &schema) const
{
  // ORDER BY does not change the tuple schema; forward schema from child
  if (!children_.empty() && children_[0] != nullptr) {
    return children_[0]->tuple_schema(schema);
  }
  return RC::SUCCESS;
}
