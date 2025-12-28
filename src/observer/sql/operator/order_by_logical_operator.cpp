#include "sql/operator/order_by_logical_operator.h"

OrderByLogicalOperator::OrderByLogicalOperator(vector<OrderByItem> &&order_by_items)
{
	order_by_items_ = std::move(order_by_items);
}
