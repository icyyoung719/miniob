#pragma once

#include "sql/operator/logical_operator.h"

class UpdateLogicalOperator : public LogicalOperator
{
public:
  UpdateLogicalOperator(Table *table, const string &attr_name, const Value &value)
      : table_(table), attribute_name_(attr_name), value_(value) {}
  virtual ~UpdateLogicalOperator() = default;

  LogicalOperatorType type() const override { return LogicalOperatorType::UPDATE; }
  OpType get_op_type() const override { return OpType::LOGICALUPDATE; }

  Table *table() const { return table_; }
  const string &attribute_name() const { return attribute_name_; }
  const Value &value() const { return value_; }

private:
  Table *table_ = nullptr;
  string attribute_name_;
  Value  value_;
};
