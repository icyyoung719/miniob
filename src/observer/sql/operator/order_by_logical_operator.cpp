#include "sql/operator/order_by_logical_operator.h"

OrderByLogicalOperator::OrderByLogicalOperator(vector<unique_ptr<Expression>> &&order_by_exprs)
{
  order_by_expressions_ = std::move(order_by_exprs);
}
