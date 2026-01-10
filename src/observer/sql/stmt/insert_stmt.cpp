/* Copyright (c) 2021OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/5/22.
//

#include "sql/stmt/insert_stmt.h"
#include "common/log/log.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

InsertStmt::InsertStmt(Table *table, const vector<vector<Value>> &rows)
  : table_(table), rows_(rows)
{}

RC InsertStmt::create(Db *db, const InsertSqlNode &inserts, Stmt *&stmt)
{
  const char *table_name = inserts.relation_name.c_str();
  if (nullptr == db || nullptr == table_name || inserts.values.empty()) {
    LOG_WARN("invalid argument. db=%p, table_name=%p, rows_num=%d",
        db, table_name, static_cast<int>(inserts.values.size()));
    return RC::INVALID_ARGUMENT;
  }

  // check whether the table exists
  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // check the fields number
  const TableMeta &table_meta = table->table_meta();
  const int        field_num  = table_meta.field_num() - table_meta.sys_field_num();

  // validate each row has correct number of fields
  for (const auto &row : inserts.values) {
    if (static_cast<int>(row.size()) != field_num) {
      LOG_WARN("schema mismatch. row value num=%d, field num in schema=%d", static_cast<int>(row.size()), field_num);
      return RC::SCHEMA_FIELD_MISSING;
    }
  }

  // 为了语法一致性，这部分提前到语法分析阶段进行失败检查
  // // 每一列的类型检查
  // for (int i = table_meta.sys_field_num(); i < table_meta.field_num(); i++) {
  //   // 如果是日期类型，检查日期是否合法
  //   if (values[i].attr_type() == AttrType::DATES && !values[i].is_date_valid()) {
  //     LOG_WARN("invalid date value: %s", values[i].to_string().c_str());
  //     return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  //   }
  // }

  // everything alright
  stmt = new InsertStmt(table, inserts.values);
  return RC::SUCCESS;
}
