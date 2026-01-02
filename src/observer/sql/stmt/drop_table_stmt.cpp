#include "sql/stmt/drop_table_stmt.h"
#include "common/log/log.h"
#include "event/sql_debug.h"
#include "storage/db/db.h"

RC DropTableStmt::create(const Db *db, const DropTableSqlNode &drop_table_sql, Stmt *&stmt)
{
  // 1. 基本参数校验
  if (db == nullptr) {
    LOG_WARN("invalid argument. db is null");
    return RC::INVALID_ARGUMENT;
  }
  if (drop_table_sql.relation_name.empty()) {
    LOG_WARN("invalid argument. relation name is empty");
    return RC::INVALID_ARGUMENT;
  }

  // 2. 语义校验：检查表是否存在
  const char *table_name = drop_table_sql.relation_name.c_str();
  if (db->find_table(table_name) == nullptr) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // 3. 创建并返回 DropTableStmt 对象
  stmt = new DropTableStmt(drop_table_sql.relation_name);
  sql_debug("drop table statement: table name %s", table_name);

  return RC::SUCCESS;
}
