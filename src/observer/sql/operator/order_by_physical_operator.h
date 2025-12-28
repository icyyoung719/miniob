/*
 * Physical operator to perform ORDER BY by materializing child output
 */

#pragma once

#include "sql/operator/physical_operator.h"
#include "sql/expr/tuple.h"
#include "sql/expr/expression.h"

class OrderByPhysicalOperator : public PhysicalOperator
{
public:
  OrderByPhysicalOperator(vector<unique_ptr<Expression>> &&order_by_exprs);
  virtual ~OrderByPhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::EXPR_VEC; }
  OpType              get_op_type() const override { return OpType::ORDERBY; }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override;
  RC tuple_schema(TupleSchema &schema) const override;

private:
  vector<unique_ptr<Expression>> order_by_expressions_;
  struct RowItem {
    ValueListTuple row;
    vector<Value>  keys;
  };
  vector<RowItem> rows_;
  size_t          cursor_ = 0;
  ValueListTuple *cur_tuple_ = nullptr;
};
