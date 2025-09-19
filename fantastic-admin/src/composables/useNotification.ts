import { faNotification as toast } from '@/ui/components/FaNotification'
import type { TodoItem } from '@/utils/websocket'

/**
 * 通知服务组合式函数
 * 用于显示各种类型的通知消息
 */
export function useNotification() {
  /**
   * 显示新待办事项通知
   */
  function showTodoNotification(todo: TodoItem) {
    console.log(`显示待办事项通知: ${todo.title} (${todo.priority})`);
    
    const priorityConfig = {
      high: {
        variant: 'destructive' as const,
        icon: 'custom-todo',
        title: '高优先级待办'
      },
      medium: {
        variant: 'default' as const,
        icon: 'custom-todo',
        title: '新待办事项'
      },
      low: {
        variant: 'default' as const,
        icon: 'custom-todo',
        title: '待办提醒'
      }
    }

    const config = priorityConfig[todo.priority] || priorityConfig.medium

    toast({
      title: config.title,
      description: `${todo.title}${todo.description ? ` - ${todo.description}` : ''}`,
      variant: config.variant,
      icon: config.icon,
      duration: todo.priority === 'high' ? 8000 : 5000,
      action: {
        altText: '查看详情',
        label: '查看',
        onClick: () => {
          // 这里可以添加跳转到待办详情的逻辑
          console.log('查看待办详情:', todo.id)
        }
      }
    })
  }

  /**
   * 显示待办事项更新通知
   */
  function showTodoUpdateNotification(todo: TodoItem, action: 'completed' | 'updated' | 'deleted') {
    const actionConfig = {
      completed: {
        title: '✅ 任务完成',
        description: `已完成：${todo.title}`,
        variant: 'default' as const,
        duration: 3000
      },
      updated: {
        title: '📝 任务更新',
        description: `已更新：${todo.title}`,
        variant: 'default' as const,
        duration: 3000
      },
      deleted: {
        title: '🗑️ 任务删除',
        description: `已删除：${todo.title}`,
        variant: 'destructive' as const,
        duration: 3000
      }
    }

    const config = actionConfig[action]

    toast({
      icon: 'custom-todo',
      title: config.title,
      description: config.description,
      variant: config.variant,
      duration: config.duration
    })
  }

  /**
   * 显示WebSocket连接状态通知
   */
  function showConnectionNotification(isConnected: boolean) {
    if (isConnected) {
      console.log(`显示连接成功通知`);
      
      // toast({
      //   title: '🔗 连接成功',
      //   description: 'WebSocket连接已建立，可以接收实时通知',
      //   variant: 'default',
      //   duration: 3000
      // })
    } else {
      console.log(`显示连接断开通知`);
      // toast({
      //   title: '⚠️ 连接断开',
      //   description: 'WebSocket连接已断开，正在尝试重连...',
      //   variant: 'destructive',
      //   duration: 5000
      // })
    }
  }

  /**
   * 显示错误通知
   */
  function showErrorNotification(title: string, description?: string) {
    toast({
      title: `❌ ${title}`,
      description: description || '操作失败，请稍后重试',
      variant: 'destructive',
      duration: 5000
    })
  }

  /**
   * 显示成功通知
   */
  function showSuccessNotification(title: string, description?: string) {
    toast({
      title: `✅ ${title}`,
      description,
      variant: 'default',
      duration: 3000
    })
  }

  /**
   * 显示信息通知
   */
  function showInfoNotification(title: string, description?: string) {
    toast({
      title: `ℹ️ ${title}`,
      description,
      variant: 'default',
      duration: 4000
    })
  }

  /**
   * 显示警告通知
   */
  function showWarningNotification(title: string, description?: string) {
    toast({
      title: `⚠️ ${title}`,
      description,
      variant: 'destructive',
      duration: 5000
    })
  }

  /**
   * 显示待办事项过期提醒
   */
  function showOverdueNotification(todos: TodoItem[]) {
    if (todos.length === 0) return

    const title = todos.length === 1 
      ? '⏰ 待办事项已过期' 
      : `⏰ ${todos.length}个待办事项已过期`
    
    const description = todos.length === 1
      ? todos[0].title
      : `${todos[0].title} 等${todos.length}个任务需要处理`

    toast({
      title,
      description,
      variant: 'destructive',
      duration: 8000,
      action: {
        altText: '查看全部',
        label: '查看',
        onClick: () => {
          // 这里可以添加跳转到待办列表的逻辑
          console.log('查看过期待办事项')
        }
      }
    })
  }

  /**
   * 显示每日待办总结通知
   */
  function showDailySummaryNotification(summary: {
    total: number
    completed: number
    pending: number
    overdue: number
  }) {
    const { total, completed, pending, overdue } = summary
    
    let title = '📊 今日待办总结'
    let description = `共${total}项任务，已完成${completed}项`
    
    if (pending > 0) {
      description += `，待处理${pending}项`
    }
    
    if (overdue > 0) {
      description += `，已过期${overdue}项`
      title = '📊 今日待办总结 ⚠️'
    }

    toast({
      title,
      description,
      variant: overdue > 0 ? 'destructive' : 'default',
      duration: 6000
    })
  }

  /**
   * 显示系统通知
   */
  function showSystemNotification(title: string, message: string, type?: 'info' | 'warning' | 'error') {
    const typeConfig = {
      info: {
        icon: 'ℹ️',
        variant: 'default' as const,
        duration: 4000
      },
      warning: {
        icon: '⚠️',
        variant: 'destructive' as const,
        duration: 5000
      },
      error: {
        icon: '❌',
        variant: 'destructive' as const,
        duration: 6000
      }
    }

    const config = typeConfig[type || 'info']

    toast({
      title: `${config.icon} ${title}`,
      description: message,
      variant: config.variant,
      duration: config.duration
    })
  }

  return {
    showTodoNotification,
    showTodoUpdateNotification,
    showConnectionNotification,
    showErrorNotification,
    showSuccessNotification,
    showInfoNotification,
    showWarningNotification,
    showOverdueNotification,
    showDailySummaryNotification,
    showSystemNotification
  }
}