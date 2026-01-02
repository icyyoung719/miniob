// src/observer/sql/stmt/drop_table_stmt.h

#pragma once

#include "common/lang/string.h"
#include "sql/stmt/stmt.h"

class Db;

/**
 * @brief 表示删除表的语句
 * @ingroup Statement
 * @details 用于存储 DROP TABLE 语句解析后的关键信息。
 */
class DropTableStmt : public Stmt
{
public:
  /**
   * @brief 构造函数
   * @param table_name 要删除的表的名称
   */
  explicit DropTableStmt(const string &table_name) : table_name_(std::move(table_name)) {}
  virtual ~DropTableStmt() override = default;

  /**
   * @brief 获取语句类型
   * @return 返回 StmtType::DROP_TABLE
   */
  StmtType type() const override { return StmtType::DROP_TABLE; }

  /**
   * @brief 获取要删除的表的名称
   * @return 返回表名的常量引用
   */
  const string &table_name() const { return table_name_; }

  /**
   * @brief 创建 DropTableStmt 实例
   * @param db 数据库对象指针
   * @param drop_table_sql 解析后的 SQL 节点
   * @param stmt 输出参数，用于存储创建的 Stmt 对象
   * @return 如果成功创建，返回 RC::SUCCESS，否则返回相应的错误码
   */
  static RC create(const Db *db, const DropTableSqlNode &drop_table_sql, Stmt *&stmt);

private:
  string table_name_; ///< 要删除的表的名称
};

