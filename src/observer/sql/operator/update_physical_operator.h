#pragma once

#include "sql/operator/physical_operator.h"

class UpdatePhysicalOperator : public PhysicalOperator
{
public:
    UpdatePhysicalOperator(Table *table, const string &attr_name, const Value &value)
      : table_(table), attribute_name_(attr_name), value_(value) {}

  virtual ~UpdatePhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::UPDATE; }
  OpType get_op_type() const override { return OpType::UPDATE; }

  RC open(Trx *trx) override;
  RC next() override { return RC::RECORD_EOF; }
  RC close() override { return RC::SUCCESS; }

  Tuple *current_tuple() override { return nullptr; }

private:
  Table *table_ = nullptr;
  string attribute_name_;
  Value  value_;
  Trx   *trx_ = nullptr;
};
