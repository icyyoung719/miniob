#include "sql/executor/drop_table_executor.h"

#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"
#include "sql/stmt/drop_table_stmt.h" // 需要包含DropTableStmt的头文件
#include "storage/db/db.h"

/**
 * @brief 执行 DROP TABLE 语句的执行器
 * @ingroup Executor
 */
RC DropTableExecutor::execute(SQLStageEvent *sql_event)
{
  // 1. 从SQL事件中获取语句(Stmt)和会话(Session)对象
  Stmt    *stmt    = sql_event->stmt();
  Session *session = sql_event->session_event()->session();

  // 2. 断言，确保当前执行器处理的是DROP_TABLE类型的语句
  ASSERT(stmt->type() == StmtType::DROP_TABLE,
         "drop table executor can not run this command: %d",
         static_cast<int>(stmt->type()));

  // 3. 将通用的Stmt指针转换为具体的DropTableStmt指针
  DropTableStmt *drop_table_stmt = static_cast<DropTableStmt *>(stmt);

  // 4. 从DropTableStmt中获取要删除的表名
  const char *table_name = drop_table_stmt->table_name().c_str();

  // 5. 获取当前会话所在的数据库，并调用其drop_table方法来执行删除操作
  //    该方法会处理表的元数据删除、数据文件删除等底层逻辑
  RC rc = session->get_current_db()->drop_table(table_name);

  // 6. 返回操作结果
  return rc;
}
