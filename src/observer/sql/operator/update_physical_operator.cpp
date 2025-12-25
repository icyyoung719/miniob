#include "sql/operator/update_physical_operator.h"
#include "sql/stmt/update_stmt.h"
#include "sql/expr/expression.h"
#include "storage/trx/trx.h"
#include "storage/table/table.h"
#include "storage/record/record_scanner.h"
#include "common/log/log.h"

RC UpdatePhysicalOperator::open(Trx *trx)
{
  trx_ = trx;

  if (children_.empty()) {
    // full table update: scan all records
    RecordScanner *scanner = nullptr;
    RC rc = table_->get_record_scanner(scanner, trx_, ReadWriteMode::READ_WRITE);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get record scanner. rc=%s", strrc(rc));
      return rc;
    }

    Record record;
    while (OB_SUCC(rc = scanner->next(record))) {
      // make a copy for old and new
      Record old_record = record;
      Record new_record = record;

      // find field meta
      const TableMeta &meta = table_->table_meta();
      const FieldMeta *field = meta.field(attribute_name_.c_str());
      if (field == nullptr) {
        scanner->close_scan();
        delete scanner;
        LOG_WARN("no such field: %s", attribute_name_.c_str());
        return RC::SCHEMA_FIELD_MISSING;
      }

      // ensure new_record owns its data so we can modify it
      RC rc2 = new_record.copy_data(old_record.data(), old_record.len());
      if (rc2 != RC::SUCCESS) {
        scanner->close_scan();
        delete scanner;
        return rc2;
      }

      // prepare value (cast if needed)
      Value real_value;
      const Value *value_ptr = &value_;
      if (field->type() != value_.attr_type()) {
        rc2 = Value::cast_to(value_, field->type(), real_value);
        if (rc2 != RC::SUCCESS) {
          scanner->close_scan();
          delete scanner;
          LOG_WARN("failed to cast value for field %s", field->name());
          return rc2;
        }
        value_ptr = &real_value;
      }

      // compute copy length similar to Table::set_value_to_record
      size_t copy_len = field->len();
      size_t data_len = value_ptr->length();
      if (field->type() == AttrType::CHARS) {
        if (copy_len > data_len) {
          copy_len = data_len + 1;
        }
      }

      rc2 = new_record.set_field(field->offset(), (int)copy_len, const_cast<char *>(value_ptr->data()));
      if (rc2 != RC::SUCCESS) {
        scanner->close_scan();
        delete scanner;
        return rc2;
      }

      rc2 = trx_->update_record(table_, old_record, new_record);
      if (rc2 != RC::SUCCESS) {
        scanner->close_scan();
        delete scanner;
        LOG_WARN("failed to update record. rc=%s", strrc(rc2));
        return rc2;
      }
    }

    if (rc == RC::RECORD_EOF) {
      rc = RC::SUCCESS;
    }
    scanner->close_scan();
    delete scanner;
    return rc;
  } else {
    // with child (predicate) - update matched records collected from child
    unique_ptr<PhysicalOperator> &child = children_[0];
    RC rc = child->open(trx_);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to open child operator: %s", strrc(rc));
      return rc;
    }

    while (OB_SUCC(rc = child->next())) {
      Tuple *tuple = child->current_tuple();
      if (nullptr == tuple) {
        LOG_WARN("failed to get current tuple: %s", strrc(rc));
        return rc;
      }
      RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
      Record old_record = row_tuple->record();
      Record new_record = old_record;

      const FieldMeta *field = table_->table_meta().field(attribute_name_.c_str());
      if (field == nullptr) {
        LOG_WARN("no such field: %s", attribute_name_.c_str());
        return RC::SCHEMA_FIELD_MISSING;
      }

      RC rc2 = new_record.copy_data(old_record.data(), old_record.len());
      if (rc2 != RC::SUCCESS) {
        return rc2;
      }

      // cast if needed
      Value real_value;
      const Value *value_ptr = &value_;
      if (field->type() != value_.attr_type()) {
        rc2 = Value::cast_to(value_, field->type(), real_value);
        if (rc2 != RC::SUCCESS) {
          LOG_WARN("failed to cast value for field %s", field->name());
          return rc2;
        }
        value_ptr = &real_value;
      }

      size_t copy_len = field->len();
      size_t data_len = value_ptr->length();
      if (field->type() == AttrType::CHARS) {
        if (copy_len > data_len) {
          copy_len = data_len + 1;
        }
      }

      rc2 = new_record.set_field(field->offset(), (int)copy_len, const_cast<char *>(value_ptr->data()));
      if (rc2 != RC::SUCCESS) {
        return rc2;
      }

      rc2 = trx_->update_record(table_, old_record, new_record);
      if (rc2 != RC::SUCCESS) {
        LOG_WARN("failed to update record. rc=%s", strrc(rc2));
        return rc2;
      }
    }

    child->close();
    return RC::SUCCESS;
  }
}
