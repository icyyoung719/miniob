/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
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

#include "sql/stmt/filter_stmt.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/sys/rc.h"
#include "storage/db/db.h"
#include "storage/table/table.h"
#include "sql/parser/expression_binder.h"

FilterStmt::~FilterStmt() {}

RC FilterStmt::create(Db *db, Table *default_table, unordered_map<string, Table *> *tables,
    const ConditionSqlNode *conditions, int condition_num, FilterStmt *&stmt)
{
  RC rc = RC::SUCCESS;
  stmt  = nullptr;

  // prepare binder context
  BinderContext binder_context;
  if (default_table != nullptr) {
    binder_context.add_table(default_table);
  }
  if (tables != nullptr) {
    for (auto &entry : *tables) {
      if (entry.second != nullptr && entry.second != default_table) {
        binder_context.add_table(entry.second);
      }
    }
  }

  ExpressionBinder binder(binder_context);

  FilterStmt *tmp_stmt = new FilterStmt();
  for (int i = 0; i < condition_num; i++) {
    const ConditionSqlNode &in = conditions[i];

    ConditionSqlNode out;
    // bind left expression
    if (in.left_expr != nullptr) {
      vector<unique_ptr<Expression>> bound_left;
      rc = binder.bind_expression(const_cast<unique_ptr<Expression>&>(const_cast<ConditionSqlNode&>(in).left_expr), bound_left);
      if (rc != RC::SUCCESS) {
        delete tmp_stmt;
        LOG_WARN("bind left expression failed. rc=%s", strrc(rc));
        return rc;
      }
      if (bound_left.size() != 1) {
        delete tmp_stmt;
        LOG_WARN("invalid bound left expression count: %zu", bound_left.size());
        return RC::INVALID_ARGUMENT;
      }
      out.left_expr = std::move(bound_left[0]);
    }

    // bind right expression
    if (in.right_expr != nullptr) {
      vector<unique_ptr<Expression>> bound_right;
      rc = binder.bind_expression(const_cast<unique_ptr<Expression>&>(const_cast<ConditionSqlNode&>(in).right_expr), bound_right);
      if (rc != RC::SUCCESS) {
        delete tmp_stmt;
        LOG_WARN("bind right expression failed. rc=%s", strrc(rc));
        return rc;
      }
      if (bound_right.size() != 1) {
        delete tmp_stmt;
        LOG_WARN("invalid bound right expression count: %zu", bound_right.size());
        return RC::INVALID_ARGUMENT;
      }
      out.right_expr = std::move(bound_right[0]);
    }

    out.comp = in.comp;
    // push copied/owned condition
    tmp_stmt->conditions_.push_back(std::move(out));
  }

  stmt = tmp_stmt;
  return RC::SUCCESS;
}

// NOTE: old helper get_table_and_field and create_filter_unit are no longer used
