import { faNotification as toast } from '@/ui/components/FaNotification'
import type { TodoItem } from '@/utils/websocket'
import { useNotificationStore } from '@/store/modules/notification'
import { ref } from 'vue'

/**
 * 格式化时间为 HH:mm:ss 格式
 */
export function formatRelativeTime(dateString: string) {
  const date = new Date(dateString)
  const hours = date.getHours().toString().padStart(2, '0')
  const minutes = date.getMinutes().toString().padStart(2, '0')
  const seconds = date.getSeconds().toString().padStart(2, '0')
  
  return `${hours}:${minutes}:${seconds}`
}

/**
 * 通知服务组合式函数
 * 用于显示各种类型的通知消息
 */
export function useNotification() {
  // 全局 toast 通知开关
  const showToastNotifications = ref(true)
  
  // 使用通知存储
  const notificationStore = useNotificationStore()
  
  // 从 localStorage 加载设置
  function loadToastSettings() {
    const saved = localStorage.getItem('notification-show-toast')
    console.log('loadToastSettings', saved);
    
    if (saved !== null) {
      showToastNotifications.value = saved === 'true'
    }
  }

  /**
   * 保存通知到store（已迁移到store）
   */
  function saveNotificationToStorage(todo: TodoItem, messageId?: string) {
    // 功能已迁移到notificationStore.saveNotification
  }

  /**
   * 从store加载通知（已迁移到store）
   */
  function loadNotificationsFromStorage(): Array<{id: string, todo: TodoItem, timestamp: string, isRead: boolean}> {
    // 功能已迁移到notificationStore.allNotifications
    return []
  }

  /**
   * 标记通知为已读（已迁移到store）
   */
  function markNotificationAsRead(notificationId: string) {
    // 功能已迁移到notificationStore.markAsRead
  }

  /**
   * 清除store中的通知（已迁移到store）
   */
  function clearStoredNotifications() {
    // 功能已迁移到notificationStore.clearAll
  }
  
  // 保存设置到 localStorage
  function saveToastSettings() {
    localStorage.setItem('notification-show-toast', showToastNotifications.value.toString())
  }
  
  // 切换 toast 通知开关
  function toggleToastNotifications() {
    showToastNotifications.value = !showToastNotifications.value
    saveToastSettings()
    
    // 显示切换状态提示
    if (showToastNotifications.value) {
      toast({
        title: '✅ 通知已开启',
        description: '通知提醒已启用',
        variant: 'success',
        duration: 2000
      })
    } else {
      toast({
        title: '🔕 通知已关闭',
        description: '通知提醒已禁用',
        variant: 'default',
        duration: 2000
      })
    }
  }
  
  // 初始化设置
  loadToastSettings()
  
  /**
   * 显示新待办事项通知
   */
  function showTodoNotification(todo: TodoItem, messageId?: string) {
    // 如果 toast 通知被禁用，则不显示
    console.log(`showToastNotifications:`, localStorage.getItem('notification-show-toast'));
    
    if (localStorage.getItem('notification-show-toast') === 'false') return
    
    // 保存通知到store（现在由调用方负责保存）
    // saveNotificationToStorage(todo, messageId)
    
    console.log(`显示待办事项通知: ${todo.title} (${todo.priority})`, messageId ? `消息ID: ${messageId}` : '');
    
    const priorityConfig = {
      success: {
        variant: 'success' as const,
        icon: 'custom-todo',
        title: '工单完成'
      },
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

    // 新的显示格式：{orderType}#{id}({modelName}){category}[{client}] + 相对时间
    const description = `${todo.orderType == '直接封装+发送' ? '版本迭代+交付发送' : todo.orderType}#${todo.id}(${todo.modelName})${todo.category}${todo.client ? `[${todo.client}]` : ''}${todo.timestamp ? ` ${formatRelativeTime(todo.timestamp)}` : ''}`;

    toast({
      title: todo.title,
      description: description,
      variant: config.variant,
      icon: config.icon,
      duration: todo.priority === 'high' ? 8000 : 5000,
      // action: {
      //   altText: '查看详情',
      //   label: '查看',
      //   onClick: () => {
      //     // 这里可以添加跳转到待办详情的逻辑
      //     console.log('查看待办详情:', todo.id)
      //   }
      // }
    })
  }

  /**
   * 显示待办事项更新通知
   */
  function showTodoUpdateNotification(todo: TodoItem, action: 'completed' | 'updated' | 'deleted') {
    // 如果 toast 通知被禁用，则不显示
    if (localStorage.getItem('notification-show-toast') === 'false') return
    
    const actionConfig = {
      completed: {
        title: '✅ 任务完成',
        description: `已完成：${todo.title}`,
        variant: 'success' as const,
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
   * 显示错误通知
   */
  function showErrorNotification(title: string, description?: string) {
    // 如果 toast 通知被禁用，则不显示
    if (localStorage.getItem('notification-show-toast') === 'false') return
    
    toast({
      title: `❌ ${title}`,
      description: description || '操作失败，请稍后重试',
      variant: 'destructive',
      duration: 5000
    })
  }

  /**
   * 显示成功通知（绿色背景）
   */
  function showSuccessNotification(title: string, description?: string) {
    // 如果 toast 通知被禁用，则不显示
    if (localStorage.getItem('notification-show-toast') === 'false') return
    
    toast({
      title: `✅ ${title}`,
      description,
      variant: 'success',
      duration: 3000
    })
  }

  /**
   * 显示信息通知
   */
  // function showInfoNotification(title: string, description?: string) {
  //   // 如果 toast 通知被禁用，则不显示
  //   if (localStorage.getItem('notification-show-toast') === 'false') return
    
  //   toast({
  //     title: `ℹ️ ${title}`,
  //     description,
  //     variant: 'default',
  //     duration: 4000
  //   })
  // }

  /**
   * 显示警告通知
   */
  function showWarningNotification(title: string, description?: string) {
    // 如果 toast 通知被禁用，则不显示
    if (localStorage.getItem('notification-show-toast') === 'false') return
    
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
    // 如果 toast 通知被禁用，则不显示
    if (localStorage.getItem('notification-show-toast') === 'false') return
    
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
    // 如果 toast 通知被禁用，则不显示
    if (localStorage.getItem('notification-show-toast') === 'false') return
    
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
    // 如果 toast 通知被禁用，则不显示
    if (localStorage.getItem('notification-show-toast') === 'false') return
    
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
    showErrorNotification,
    showSuccessNotification,
    // showInfoNotification,
    showWarningNotification,
    showOverdueNotification,
    showDailySummaryNotification,
    showSystemNotification,
    toggleToastNotifications,
    showToastNotifications,
    // 本地存储相关方法
    saveNotificationToStorage,
    loadNotificationsFromStorage,
    markNotificationAsRead,
    clearStoredNotifications
  }
}