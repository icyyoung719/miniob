/*
 * OrderBy physical operator implementation
 */

#include "sql/operator/order_by_physical_operator.h"
#include "common/log/log.h"

using namespace std;

OrderByPhysicalOperator::OrderByPhysicalOperator(vector<unique_ptr<Expression>> &&order_by_exprs)
  : order_by_expressions_(std::move(order_by_exprs)), cursor_(0), cur_tuple_(nullptr)
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
    for (auto &expr : order_by_expressions_) {
      Value key;
      RC krc = expr->get_value(*t, key);
      if (krc != RC::SUCCESS) {
        // if cannot get key, push a null/empty value so comparisons still work
        key.reset();
      }
      item.keys.push_back(key);
    }

    rows_.push_back(std::move(item));
  }

  // sort rows by precomputed keys
  if (!order_by_expressions_.empty()) {
    sort(rows_.begin(), rows_.end(), [&](const RowItem &a, const RowItem &b) {
      for (size_t i = 0; i < a.keys.size() && i < b.keys.size(); ++i) {
        int cmp = a.keys[i].compare(b.keys[i]);
        if (cmp < 0) {
          return true;
        } else if (cmp > 0) {
          return false;
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
  if (rows_.empty()) {
    return RC::SUCCESS;
  }
  const ValueListTuple &row = rows_.front().row;
  const int cell_num = row.cell_num();
  for (int i = 0; i < cell_num; i++) {
    TupleCellSpec spec;
    if (OB_FAIL(row.spec_at(i, spec))) {
      continue;
    }
    schema.append_cell(spec);
  }
  return RC::SUCCESS;
}
