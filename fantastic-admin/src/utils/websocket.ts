import { ref, reactive } from 'vue'
import { useNotification } from '@/composables/useNotification'
import { useUserStore } from '@/store/modules/user'

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
  description: string
  priority: TodoPriority
  status: Exclude<TodoStatus, TodoStatus.ALL>
  createdAt: string
  dueDate?: string
  category?: string
  userId?: string // 待办事项所有者ID
}

// WebSocket消息类型
export interface WebSocketMessage {
  type: 'todo_notification' | 'todo_update' | 'todo_delete' | 'system_notification' | 'auth' | 'auth_success' | 'auth_failed' | 'ping' | 'pong'
  data: any
  timestamp: string
  userId?: string // 消息发送者ID
  account?: string // 消息发送者账号
}

// WebSocket服务类
class WebSocketService {
  private ws: WebSocket | null = null
  private reconnectAttempts = 0
  private maxReconnectAttempts = 5
  private reconnectInterval = 3000
  private heartbeatInterval: number | null = null
  private url: string
  private notification = useNotification()

  // 响应式状态
  public isConnected = ref(false)
  public todos = reactive<TodoItem[]>([])
  public unreadCount = ref(0)
  public isAuthenticated = ref(false)
  public currentUser = ref<{ userId: string; account: string; role?: string } | null>(null)
  private isInitialLoad = true // 标记是否为初始加载

  constructor(url: string = 'ws://localhost:8080/ws') {
    this.url = url
  }

  // 连接WebSocket
  connect() {
    try {
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
      this.reconnectAttempts = 0
      this.startHeartbeat()
      
      // 自动进行用户认证
      this.authenticateUser()
      
      // 显示连接成功通知
      if (this.reconnectAttempts > 0) {
        this.notification.showConnectionNotification(true)
      }
    }

    this.ws.onmessage = (event) => {
      try {
        const message: WebSocketMessage = JSON.parse(event.data)
        console.log('收到WebSocket消息:', message);
        
        this.handleMessage(message)
      } catch (error) {
        console.error('解析WebSocket消息失败:', error)
        this.notification.showErrorNotification('消息解析失败', '收到的消息格式不正确')
      }
    }

    this.ws.onclose = (event) => {
      console.log('WebSocket连接已关闭:', event.code, event.reason)
      this.isConnected.value = false
      this.stopHeartbeat()
      
      // 显示连接断开通知
      this.notification.showConnectionNotification(false)
      
      if (event.code !== 1000) { // 非正常关闭
        this.scheduleReconnect()
      }
    }

    this.ws.onerror = (error) => {
      console.error('WebSocket错误:', error)
      this.isConnected.value = false
      
      // this.notification.showErrorNotification('连接错误', 'WebSocket连接出现问题')
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
      case 'pong':
        // 心跳响应，无需特殊处理
        break
      case 'todo_notification':
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
    // 添加到待办列表
    this.todos.push(data)
    
    // 只有在非初始加载时才增加未读计数和显示通知
    if (!this.isInitialLoad) {
      // 只有pending状态的待办事项才计入未读
      if (data.status === 'pending') {
        this.unreadCount.value++
      }
      
      // 显示新待办事项通知
      this.notification.showTodoNotification(data)
      
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
  }

  // 用户认证
  private authenticateUser() {
    // 从用户存储中获取当前用户信息
    const userStore = useUserStore()
    
    if (userStore.account && userStore.isLogin) {
      this.send({
        type: 'auth',
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
    const index = this.todos.findIndex(todo => todo.id === data.id)
    if (index !== -1) {
      const oldTodo = this.todos[index]
      Object.assign(this.todos[index], data)
      
      // 显示更新通知
      if (oldTodo.status !== data.status && data.status === 'completed') {
        this.notification.showTodoUpdateNotification(data, 'completed')
      } else {
        this.notification.showTodoUpdateNotification(data, 'updated')
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
    const index = this.todos.findIndex(todo => todo.id === data.id)
    if (index !== -1) {
      const todo = this.todos[index]
      this.todos.splice(index, 1)
      
      // 显示删除通知
      this.notification.showTodoUpdateNotification(todo, 'deleted')
      
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
      this.ws.send(JSON.stringify(message))
    } else {
      console.warn('WebSocket未连接，无法发送消息')
    }
  }

  // 标记待办事项为已读
  markAsRead(todoId?: string) {
    if (todoId) {
      const todo = this.todos.find(t => t.id === todoId)
      if (todo && todo.status === 'pending') {
        this.unreadCount.value = Math.max(0, this.unreadCount.value - 1)
      }
    } else {
      // 标记所有为已读
      this.unreadCount.value = 0
    }
  }

  // 完成待办事项
  completeTodo(todoId: string) {
    const todo = this.todos.find(t => t.id === todoId)
    if (todo) {
      todo.status = TodoStatus.COMPLETED
      this.send({
        type: 'todo_complete',
        data: { id: todoId }
      })
    }
  }

  // 删除待办事项
  deleteTodo(todoId: string) {
    this.send({
      type: 'todo_delete',
      data: { id: todoId }
    })
  }

  // 心跳检测
  private startHeartbeat() {
    this.heartbeatInterval = window.setInterval(() => {
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        this.send({ type: 'ping' })
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
    if (this.reconnectAttempts < this.maxReconnectAttempts) {
      this.reconnectAttempts++
      console.log(`尝试重连 (${this.reconnectAttempts}/${this.maxReconnectAttempts})...`)
      
      setTimeout(() => {
        this.connect()
      }, this.reconnectInterval)
    } else {
      console.error('达到最大重连次数，停止重连')
    }
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
  }

  // 重置重连计数
  resetReconnectAttempts() {
    this.reconnectAttempts = 0
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