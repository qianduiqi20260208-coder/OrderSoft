import { ref, reactive } from 'vue'
import { useNotification } from '@/composables/useNotification'
import { useUserStore } from '@/store/modules/user'
import { useNotificationStore } from '@/store/modules/notification'

// 待办事项状态枚举
export enum TodoStatus {
  ALL = 'all',
  PENDING = 'pending',
  IN_PROGRESS = 'in_progress',
  COMPLETED = 'completed'
}

// 待办事项优先级枚举
export enum TodoPriority {
  HIGH = 'high',
  MEDIUM = 'medium',
  LOW = 'low'
}

// 待办事项接口定义
export interface TodoItem {
  id: string
  title: string
  orderType: string // 订单类型，如"交付发送"
  modelName: string // 模型名称，如"质量特性仿真模型"
  priority: TodoPriority
  status: Exclude<TodoStatus, TodoStatus.ALL>
  createdAt: string
  dueDate?: string
  category: string // 处理阶段：待分发/待审批/待封装/待加密/待发送/待完成
  userId?: string // 待办事项所有者ID
  timestamp?: string // 通知时间戳，用于显示相对时间
  messageId?: string // 关联的消息ID，用于追踪消息来源
  client?: string // 客户名称
}

// WebSocket消息类型
export interface WebSocketMessage {
  type: 'todo_notification' | 'todo_update' | 'todo_delete' | 'system_notification' | 'auth' | 'auth_success' | 'auth_failed' | 'ping' | 'pong'
  data: any
  timestamp: string
  userId?: string // 消息发送者ID
  account?: string // 消息发送者账号
  messageId?: string // 消息ID，用于唯一标识每条消息
}

// WebSocket服务类
class WebSocketService {
  private ws: WebSocket | null = null
  private reconnectAttempts = 0
  private reconnectInterval = 3000
  private maxReconnectAttempts = 999
  private reconnectMultiplier = 1.5
  private heartbeatInterval: number | null = null
  private url: string
  private notification = useNotification()
  private userStore = useUserStore()
  private notificationStore = useNotificationStore()

  // 响应式状态
  public isConnected = ref(false)
  public todos = reactive<TodoItem[]>([])
  public unreadCount = ref(0)
  public isAuthenticated = ref(false)
  public currentUser = ref<{ userId: string; account: string; role?: string } | null>(null)
  private isInitialLoad = true // 标记是否为初始加载

  // constructor(url: string = 'ws://localhost:18080/ws') {
  constructor(url: string = 'ws://10.20.28.63:18080/ws') {
    this.url = url
  }

  // 连接WebSocket
  connect() {
    try {
      // 如果已经存在连接，先关闭它
      if (this.ws) {
        this.ws.close(1000, '重新连接')
        this.ws = null
      }
      
      console.log(`正在连接WebSocket (尝试次数: ${this.reconnectAttempts + 1})...`)
      this.ws = new WebSocket(this.url)
      this.setupEventListeners()
    } catch (error) {
      console.error('WebSocket连接失败:', error)
      this.scheduleReconnect()
    }
  }

  // 设置事件监听器
  private setupEventListeners() {
    if (!this.ws) return

    this.ws.onopen = () => {
      console.log('WebSocket连接已建立')
      this.isConnected.value = true
      
      // 重置重连计数
      this.reconnectAttempts = 0
      
      this.startHeartbeat()
      
      // 自动进行用户认证
      this.authenticateUser()
    }

    this.ws.onmessage = (event) => {
      try {
        console.log('收到原始WebSocket数据:', event.data);
        const message: WebSocketMessage = JSON.parse(event.data)
        
        this.handleMessage(message)
      } catch (error) {
        console.error('解析WebSocket消息失败:', error)
        console.error('原始数据:', event.data)
        // this.notification.showErrorNotification('消息解析失败', '收到的消息格式不正确')
      }
    }

    this.ws.onclose = (event) => {
      console.log('WebSocket连接已关闭:', event.code, event.reason)
      this.isConnected.value = false
      this.stopHeartbeat()
      
      // 只有在非正常关闭时才显示断开通知和尝试重连
      if (event.code !== 1000) {
        console.log('WebSocket非正常关闭，准备重连...')
      } else {
        console.log('WebSocket正常关闭')
      }
      this.scheduleReconnect()
    }

    this.ws.onerror = (error) => {
      console.error('WebSocket错误:', error)
      this.isConnected.value = false
    }
  }

  // 处理接收到的消息
  private handleMessage(message: WebSocketMessage) {
    switch (message.type) {
      case 'auth_success':
        this.handleAuthSuccess(message.data)
        break
      case 'auth_failed':
        this.handleAuthFailed(message.data)
        break
      case 'ping':
        // 心跳响应，设置用户待处理工单数量        
        this.userStore.pendingWorkOrdersCount = message.data.pendingCount
        console.log('收到心跳响应，待处理工单数量:', message.data.pendingCount);
        
        break
      case 'todo_notification':
        console.log('收到待办事项通知:', message.data);
        
        this.handleTodoNotification(message.data, message)
        break
      case 'todo_update':
        this.handleTodoUpdate(message.data, message)
        break
      case 'todo_delete':
        this.handleTodoDelete(message.data, message)
        break
      case 'system_notification':
        this.handleSystemNotification(message.data)
        break
      default:
        console.warn('未知的消息类型:', message.type)
    }
  }

  // 处理待办事项通知
  private handleTodoNotification(data: TodoItem, message: WebSocketMessage) {
    // 检查是否已经存在相同ID的待办事项
    const existingTodo = this.todos.find(todo => todo.messageId === message.messageId)
    if (existingTodo) {
      console.log('待办事项已存在，跳过添加:', data.id)
      return
    }
    
    // 创建包含消息ID的待办事项数据
    const todoWithMessageId = {
      ...data,
      messageId: message.messageId
    }
    
    // 添加到待办列表
    this.todos.push(todoWithMessageId)
    
    // 保存通知到store（新通知默认未读）
      if (message.messageId) {
        this.notificationStore.saveNotification(todoWithMessageId, message.messageId, false)
      }
    
    // 只有在非初始加载时才增加未读计数和显示通知
    if (!this.isInitialLoad) {
      // 只有pending状态的待办事项才计入未读
      if (data.status === 'pending') {
        this.unreadCount.value++
      }
      
      // 显示新待办事项通知，包含消息ID
      
      // 注释掉重复的系统通知，避免显示带图标的重复通知
      // if (!this.isCurrentUserMessage(message)) {
        //   const operatorName = message.account || '其他用户'
        //   this.notification.showSystemNotification(
      //     '新待办事项',
      //     `${operatorName} 创建了新的待办事项: ${data.title}`,
      //     'info'
      //   )
      // }
    } else {
      // 初始加载时，计算pending状态的待办事项数量
      if (data.status === 'pending') {
        this.unreadCount.value++
      }
    }
    this.notification.showTodoNotification({...todoWithMessageId, timestamp: message.timestamp}, message.messageId)
  }

  // 用户认证
  private authenticateUser() {
    // 从用户存储中获取当前用户信息
    const userStore = useUserStore()
    
    if (userStore.account && userStore.isLogin) {
      this.send({
        type: 'auth',
        userId: userStore.account,
        messageId: `auth_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
        data: {
          userId: userStore.account,
          account: userStore.account,
          role: userStore.role
        },
        timestamp: new Date().toISOString()
      })
    }
  }

  // 处理认证成功
  private handleAuthSuccess(data: any) {
    this.isAuthenticated.value = true
    this.currentUser.value = data
    console.log('用户认证成功:', data)
    
    // 从本地存储加载通知
    this.loadStoredNotifications()
    
    // 设置一个延时，让初始待办事项加载完成后再标记为非初始加载
    setTimeout(() => {
      this.isInitialLoad = false
    }, 1000)
  }

  // 处理认证失败
  private handleAuthFailed(data: any) {
    this.isAuthenticated.value = false
    this.currentUser.value = null
    console.warn('用户认证失败:', data)
  }

  // 检查消息是否来自当前用户
  public isCurrentUserMessage(message: WebSocketMessage): boolean {
    if (!this.currentUser.value || !message.userId) {
      return false
    }
    return message.userId === this.currentUser.value.userId
  }

  // 处理待办事项更新
  private handleTodoUpdate(data: TodoItem, message: WebSocketMessage) {
    const index = this.todos.findIndex(todo => todo.id == data.id)
    if (index !== -1) {
      const oldTodo = this.todos[index]
      // 保留原有的messageId，如果更新数据中没有提供
      const updatedData = {
        ...data,
        messageId: data.messageId || oldTodo.messageId || message.messageId
      }
      Object.assign(this.todos[index], updatedData)
      
      // 显示更新通知
      if (oldTodo.status !== data.status && data.status === 'completed') {
        this.notification.showTodoUpdateNotification(updatedData, 'completed')
      } else {
        this.notification.showTodoUpdateNotification(updatedData, 'updated')
      }
      
      // 保存通知到store（更新通知保持原有状态）
      if (message.messageId) {
        this.notificationStore.saveNotification(updatedData, message.messageId, true)
      }
      
      // 如果待办事项已完成，可以选择删除相关通知（可选）
      if (data.status === 'completed' && oldTodo.messageId) {
        // this.notificationStore.deleteNotification(oldTodo.messageId) // 删除完成通知
      }
    }
    
    // 如果不是当前用户的操作，显示通知
    if (!this.isCurrentUserMessage(message)) {
      const operatorName = message.account || '其他用户'
      this.notification.showSystemNotification(
        '待办事项已更新',
        `${operatorName} 更新了待办事项: ${data.title}`,
        'info'
      )
    }
  }

  // 处理待办事项删除
  private handleTodoDelete(data: { id: string }, message: WebSocketMessage) {
    const index = this.todos.findIndex(todo => todo.id == data.id)
    if (index !== -1) {
      const todo = this.todos[index]
      this.todos.splice(index, 1)
      
      // 显示删除通知
      this.notification.showTodoUpdateNotification(todo, 'deleted')
      
      // 保存通知到store（删除通知保持原有状态）
      if (message.messageId) {
        this.notificationStore.saveNotification(todo, message.messageId, true)
      }
      
      // 删除相关通知（可选，因为待办事项已删除）
      if (todo.messageId) {
        // this.notificationStore.deleteNotification(todo.messageId) // 删除相关通知
      }
      
      // 如果不是当前用户的操作，显示通知
      if (!this.isCurrentUserMessage(message)) {
        const operatorName = message.account || '其他用户'
        this.notification.showSystemNotification(
          '待办事项已删除',
          `${operatorName} 删除了待办事项: ${todo.title}`,
          'warning'
        )
      }
    }
  }

  // 处理系统通知
  private handleSystemNotification(data: { title: string; message: string; type?: 'info' | 'warning' | 'error' }) {
    if (data.type === 'error') {
      this.notification.showErrorNotification(data.title, data.message)
    } else {
      this.notification.showSystemNotification(data.title, data.message, data.type)
    }
  }

  // 发送消息
  send(message: any) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      const jsonMessage = JSON.stringify(message)
      console.log(`发送消息:`,jsonMessage);
      
      this.ws.send(jsonMessage)
    } else {
      console.warn('WebSocket未连接，无法发送消息')
    }
  }

  // 标记待办事项为已读
  markAsRead(todoId?: string) {
    console.log('markAsRead', todoId);
    
    if (todoId) {
      const todo = this.todos.find(t => t.messageId == todoId)
      if (todo) {
        this.unreadCount.value = Math.max(0, this.unreadCount.value - 1)
        
        // 同时标记store中的通知为已读
        if (todo.messageId) {
          this.notificationStore.markAsRead(todo.messageId)
          this.send({type:"ack",id:todo.messageId})
          // 可选：从缓存中删除已读通知（避免存储过多已读通知）
          this.notificationStore.deleteNotification(todo.messageId) // 删除特定通知
          // this.notificationStore.clearReadNotifications() // 或者清除所有已读通知
          // 从this.todos中删除
          const index = this.todos.findIndex(t => t.messageId == todoId)
          if (index !== -1) {
            this.todos.splice(index, 1)
          }
        }
      }
    } else {
      // 标记所有为已读
      this.unreadCount.value = 0
      
      // 标记所有store中的通知为已读
      this.todos.forEach(todo => {
        if (todo.messageId) {
          this.notificationStore.markAsRead(todo.messageId)
        }
      })
    }
  }

  // 完成待办事项
  completeTodo(todoId: string) {
    const todo = this.todos.find(t => t.id === todoId)
    if (todo) {
      todo.status = TodoStatus.COMPLETED
      this.send({
        type: 'todo_complete',
        messageId: `complete_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
        data: { id: todoId }
      })
    }
  }

  // 删除待办事项
  deleteTodo(todoId: string) {
    this.send({
      type: 'todo_delete',
      messageId: `delete_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
      data: { id: todoId }
    })
  }

  // 心跳检测
  private startHeartbeat() {
    this.heartbeatInterval = window.setInterval(() => {
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        // 从store获取当前登录用户的userId,用于心跳包认证
        const userId = this.userStore.account
        if (userId) {
          console.log('发送心跳包',userId);
          this.send({ type: 'pong', userId: userId })
        }
      }
    }, 30000) // 每30秒发送一次心跳
  }

  private stopHeartbeat() {
    if (this.heartbeatInterval) {
      clearInterval(this.heartbeatInterval)
      this.heartbeatInterval = null
    }
  }

  // 重连机制
  private scheduleReconnect() {
    this.reconnectAttempts++
    
    if (this.reconnectAttempts > this.maxReconnectAttempts) {
      console.error(`重连失败：已达到最大重连次数 (${this.maxReconnectAttempts}次)`)
      this.notification.showErrorNotification(
        '连接失败', 
        `无法连接到服务器，请检查网络连接后刷新页面`
      )
      return
    }
    
    // 指数退避算法，最大间隔不超过30秒
    const delay = Math.min(
      this.reconnectInterval * Math.pow(this.reconnectMultiplier, this.reconnectAttempts - 1),
      30000
    )
    
    console.log(`尝试重连 (第${this.reconnectAttempts}次)，${delay/1000}秒后重试...`)
    
    setTimeout(() => {
      this.connect()
    }, delay)
  }

  // 断开连接
  disconnect() {
    this.stopHeartbeat()
    if (this.ws) {
      this.ws.close(1000, '主动断开连接')
      this.ws = null
    }
    this.isConnected.value = false
    this.isAuthenticated.value = false
    this.currentUser.value = null
    this.isInitialLoad = true // 重置初始加载标志
    this.unreadCount.value = 0 // 重置未读计数
    this.todos.splice(0) // 清空待办列表
    this.reconnectAttempts = 0 // 重置重连计数
    
    // 可选：断开连接时清除store中的通知（可以根据需求决定是否启用）
    // this.notificationStore.clearAll()
  }

  // 重置重连计数
  resetReconnectAttempts() {
    this.reconnectAttempts = 0
  }

  // 从本地存储加载通知
  private loadStoredNotifications() {
    try {
      // 从store加载通知
      this.notificationStore.loadFromLocalStorage()
      const notifications = this.notificationStore.notifications
      
      console.log(`从Vuex存储加载 ${notifications.length} 条通知`)
      
      // 将存储的通知添加到待办列表中（只添加未读的通知）
      const unreadNotifications = notifications.filter(n => !n.isRead)
      
      unreadNotifications.forEach(notification => {
        // 检查是否已存在相同ID的待办事项
        const existingTodo = this.todos.find(todo => todo.id === notification.todo.id)
        if (!existingTodo) {
          // 添加到待办列表
          this.todos.push(notification.todo)
          
          // 如果是pending状态，增加未读计数
          if (notification.todo.status === 'pending') {
            this.unreadCount.value++
          }
          
          console.log(`恢复Vuex通知: ${notification.todo.title}`)
        }
      })
      
      // 显示恢复通知的提示
      if (unreadNotifications.length > 0) {
       console.log(`恢复 ${unreadNotifications.length} 条未读通知`);
       
      }
    } catch (error) {
      console.error('从Vuex存储加载通知失败:', error)
    }
  }
}

// 创建全局WebSocket服务实例
export const websocketService = new WebSocketService()

// 导出便捷的组合式函数
export function useWebSocket() {
  return {
    isConnected: websocketService.isConnected,
    todos: websocketService.todos,
    unreadCount: websocketService.unreadCount,
    connect: () => websocketService.connect(),
    disconnect: () => websocketService.disconnect(),
    markAsRead: (todoId?: string) => websocketService.markAsRead(todoId),
    completeTodo: (todoId: string) => websocketService.completeTodo(todoId),
    deleteTodo: (todoId: string) => websocketService.deleteTodo(todoId),
  }
}