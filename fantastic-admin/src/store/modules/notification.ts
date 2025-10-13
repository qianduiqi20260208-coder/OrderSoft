import type { TodoItem } from '@/utils/websocket'

// 通知项接口
export interface NotificationItem {
  id: string
  todo: TodoItem
  timestamp: string
  isRead: boolean
}

export const useNotificationStore = defineStore(
  // 唯一ID
  'notification',
  () => {
    // 通知列表
    const notifications = ref<NotificationItem[]>([])
    
    // 最大存储数量
    const MAX_STORED_NOTIFICATIONS = 50
    
    // 未读通知数量
    const unreadCount = computed(() => {
      return notifications.value.filter(n => !n.isRead).length
    })
    
    // 获取所有通知
    const allNotifications = computed(() => notifications.value)
    
    // 获取未读通知
    const unreadNotifications = computed(() => {
      return notifications.value.filter(n => !n.isRead)
    })
    
    // 保存通知到存储
    function saveNotification(todo: TodoItem, messageId?: string, isRead: boolean = false) {
      try {
        // 检查是否已经存在相同的通知（根据消息ID或待办事项ID）
        const existingNotification = messageId 
          ? notifications.value.find(n => n.id === messageId)
          : notifications.value.find(n => n.todo.id === todo.id)
        
        if (existingNotification) {
          console.log('通知已存在，跳过保存:', messageId || todo.id)
          return
        }
        
        const notification: NotificationItem = {
          id: messageId || `store_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
          todo: todo,
          timestamp: new Date().toISOString(),
          isRead: isRead
        }
        
        // 添加到开头（最新的在前）
        notifications.value.unshift(notification)
        
        // 限制数量，防止存储过多
        if (notifications.value.length > MAX_STORED_NOTIFICATIONS) {
          notifications.value.splice(MAX_STORED_NOTIFICATIONS)
        }
        
        console.log('通知已保存到Vue Store:', notification.id)
      } catch (error) {
        console.error('保存通知到Vue Store失败:', error)
      }
    }
    
    // 标记通知为已读
    function markAsRead(notificationId: string) {
      try {
        const notification = notifications.value.find(n => n.id == notificationId)
        if (notification) {
          notification.isRead = true
          console.log('通知已标记为已读:', notificationId)
        }
      } catch (error) {
        console.error('标记通知为已读失败:', error)
      }
    }
    
    // 标记所有通知为已读
    function markAllAsRead() {
      try {
        notifications.value.forEach(notification => {
          notification.isRead = true
        })
        console.log('所有通知已标记为已读')
      } catch (error) {
        console.error('标记所有通知为已读失败:', error)
      }
    }
    
    // 清除所有通知
    function clearAll() {
      try {
        notifications.value = []
        console.log('所有通知已清除')
      } catch (error) {
        console.error('清除通知失败:', error)
      }
    }
    
    // 清除已读通知
    function clearReadNotifications() {
      try {
        notifications.value = notifications.value.filter(n => !n.isRead)
        console.log('已读通知已清除')
      } catch (error) {
        console.error('清除已读通知失败:', error)
      }
    }
    
    // 根据待办事项ID查找通知
    function findNotificationByTodoId(todoId: string): NotificationItem | undefined {
      return notifications.value.find(n => n.todo.id === todoId)
    }
    
    // 根据通知ID查找通知
    function findNotificationById(notificationId: string): NotificationItem | undefined {
      return notifications.value.find(n => n.id === notificationId)
    }
    
    // 根据通知ID删除通知
    function deleteNotification(notificationId: string) {
      try {
        const index = notifications.value.findIndex(n => n.id == notificationId)
        if (index !== -1) {
          notifications.value.splice(index, 1)
          console.log('通知已删除:', notificationId)
          return true
        }
        return false
      } catch (error) {
        console.error('删除通知失败:', error)
        return false
      }
    }
    
    // 根据待办事项ID删除通知
    function deleteNotificationByTodoId(todoId: string) {
      try {
        const index = notifications.value.findIndex(n => n.todo.id === todoId)
        if (index !== -1) {
          notifications.value.splice(index, 1)
          console.log('待办事项通知已删除:', todoId)
          return true
        }
        return false
      } catch (error) {
        console.error('删除待办事项通知失败:', error)
        return false
      }
    }
    
    // 从本地存储加载通知（用于持久化）
    function loadFromLocalStorage() {
      try {
        const stored = localStorage.getItem('fantastic-admin-notifications')
        if (stored) {
          const parsed = JSON.parse(stored)
          if (Array.isArray(parsed)) {
            notifications.value = parsed
            console.log(`从本地存储加载 ${parsed.length} 条通知到Vue Store`)
          }
        }
      } catch (error) {
        console.error('从本地存储加载通知失败:', error)
        notifications.value = []
      }
    }
    
    // 保存到本地存储（用于持久化）
    function saveToLocalStorage() {
      try {
        localStorage.setItem('fantastic-admin-notifications', JSON.stringify(notifications.value))
      } catch (error) {
        console.error('保存通知到本地存储失败:', error)
      }
    }
    
    // 监听通知变化并自动保存到本地存储
    watch(notifications, () => {
      saveToLocalStorage()
    }, { deep: true })
    
    return {
      notifications,
      unreadCount,
      allNotifications,
      unreadNotifications,
      saveNotification,
      markAsRead,
      markAllAsRead,
      clearAll,
      clearReadNotifications,
      findNotificationByTodoId,
      findNotificationById,
      deleteNotification,
      deleteNotificationByTodoId,
      loadFromLocalStorage,
      saveToLocalStorage
    }
  }
)