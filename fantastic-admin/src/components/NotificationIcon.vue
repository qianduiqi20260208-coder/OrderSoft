<script setup lang="ts">
import { useWebSocket } from '@/utils/websocket'
import TodoList from './TodoList.vue'

const { isConnected, unreadCount, todos, markAsRead } = useWebSocket()
const router = useRouter()

// 下拉菜单状态
const isDropdownOpen = ref(false)

// 最近的待办事项（最多显示5个）
const recentTodos = computed(() => {
  return todos
    .filter(todo => todo.status !== 'completed')
    .sort((a, b) => new Date(b.createdAt).getTime() - new Date(a.createdAt).getTime())
    .slice(0, 5)
})

// 点击通知图标
function handleNotificationClick() {
  isDropdownOpen.value = !isDropdownOpen.value
  
  // 如果打开下拉菜单，标记为已读
  if (isDropdownOpen.value && unreadCount.value > 0) {
    markAsRead()
  }
}

// 关闭下拉菜单
function closeDropdown() {
  isDropdownOpen.value = false
}

// 跳转到首页
function goToTodoPage() {
  closeDropdown()
  // 跳转到首页
  router.push('/')
}

// 点击外部关闭下拉菜单
function handleClickOutside(event: Event) {
  const target = event.target as HTMLElement
  const dropdown = document.querySelector('.notification-dropdown')
  const button = document.querySelector('.notification-button')
  
  if (dropdown && button && !dropdown.contains(target) && !button.contains(target)) {
    closeDropdown()
  }
}

// 监听点击外部事件
onMounted(() => {
  document.addEventListener('click', handleClickOutside)
})

onUnmounted(() => {
  document.removeEventListener('click', handleClickOutside)
})

// 格式化时间为相对时间
function formatRelativeTime(dateString: string) {
  const date = new Date(dateString)
  const now = new Date()
  const diffInSeconds = Math.floor((now.getTime() - date.getTime()) / 1000)
  
  if (diffInSeconds < 60) {
    return '刚刚'
  } else if (diffInSeconds < 3600) {
    return `${Math.floor(diffInSeconds / 60)}分钟前`
  } else if (diffInSeconds < 86400) {
    return `${Math.floor(diffInSeconds / 3600)}小时前`
  } else {
    return `${Math.floor(diffInSeconds / 86400)}天前`
  }
}

// 获取优先级颜色
function getPriorityColor(priority: string) {
  switch (priority) {
    case 'high':
      return 'text-red-600'
    case 'medium':
      return 'text-yellow-600'
    case 'low':
      return 'text-green-600'
    default:
      return 'text-gray-600'
  }
}
</script>

<template>
  <div class="relative">
    <!-- 通知图标按钮 -->
    <FaButton
      variant="ghost"
      size="icon"
      class="notification-button relative size-9"
      @click="handleNotificationClick"
    >
      <!-- 连接状态指示器 -->
      <div 
        class="absolute -top-1 -left-1 w-3 h-3 rounded-full border-2 border-white"
        :class="isConnected ? 'bg-green-400' : 'bg-red-400'"
        :title="isConnected ? 'WebSocket已连接' : 'WebSocket未连接'"
      />
      
      <!-- 通知图标 -->
      <FaIcon 
        name="i-material-symbols:notifications-outline" 
        class="size-5"
        :class="unreadCount > 0 ? 'text-blue-600' : 'text-gray-600'"
      />
      
      <!-- 未读消息数量徽章 -->
      <div 
        v-if="unreadCount > 0"
        class="absolute -top-2 -right-2 min-w-5 h-5 bg-red-500 text-white text-xs font-bold rounded-full flex items-center justify-center px-1"
      >
        {{ unreadCount > 99 ? '99+' : unreadCount }}
      </div>
    </FaButton>

    <!-- 下拉菜单 -->
    <Transition
      enter-active-class="transition ease-out duration-200"
      enter-from-class="opacity-0 scale-95"
      enter-to-class="opacity-100 scale-100"
      leave-active-class="transition ease-in duration-150"
      leave-from-class="opacity-100 scale-100"
      leave-to-class="opacity-0 scale-95"
    >
      <div
        v-if="isDropdownOpen"
        class="notification-dropdown absolute right-0 top-full mt-2 w-96 bg-white rounded-lg shadow-lg border border-gray-200 z-50"
      >
        <!-- 头部 -->
        <div class="p-4 border-b border-gray-200">
          <div class="flex items-center justify-between">
            <h3 class="text-lg font-semibold text-gray-900 flex items-center">
              <FaIcon name="i-material-symbols:notifications-outline" class="mr-2 size-5" />
              通知中心
            </h3>
            <div class="flex items-center gap-2">
              <!-- 连接状态 -->
              <div class="flex items-center text-xs">
                <div 
                  class="w-2 h-2 rounded-full mr-1"
                  :class="isConnected ? 'bg-green-400' : 'bg-red-400'"
                />
                <span :class="isConnected ? 'text-green-600' : 'text-red-600'">
                  {{ isConnected ? '已连接' : '未连接' }}
                </span>
              </div>
              <FaButton
                variant="ghost"
                size="icon"
                class="size-6"
                @click="closeDropdown"
              >
                <FaIcon name="i-material-symbols:close" class="size-4" />
              </FaButton>
            </div>
          </div>
        </div>

        <!-- 待办事项列表 -->
        <div class="max-h-96 overflow-y-auto">
          <div v-if="recentTodos.length === 0" class="p-6 text-center">
            <FaIcon name="i-material-symbols:task-outline" class="mx-auto size-12 text-gray-300 mb-3" />
            <h4 class="text-sm font-medium text-gray-900 mb-1">暂无待办事项</h4>
            <p class="text-xs text-gray-500">所有任务都已完成！</p>
          </div>

          <div v-else class="divide-y divide-gray-100">
            <div
              v-for="todo in recentTodos"
              :key="todo.id"
              class="p-4 hover:bg-gray-50 transition-colors cursor-pointer"
            >
              <div class="flex items-start gap-3">
                <!-- 优先级指示器 -->
                <div 
                  class="w-2 h-2 rounded-full mt-2 flex-shrink-0"
                  :class="{
                    'bg-red-400': todo.priority === 'high',
                    'bg-yellow-400': todo.priority === 'medium',
                    'bg-green-400': todo.priority === 'low'
                  }"
                />
                
                <div class="flex-1 min-w-0">
                  <!-- 标题 -->
                  <h4 class="text-sm font-medium text-gray-900 truncate">
                    {{ todo.title }}
                  </h4>
                  
                  <!-- 描述 -->
                  <p v-if="todo.description" class="text-xs text-gray-600 mt-1 line-clamp-2">
                    {{ todo.description }}
                  </p>
                  
                  <!-- 时间和分类 -->
                  <div class="flex items-center gap-2 mt-2">
                    <span class="text-xs text-gray-500">
                      {{ formatRelativeTime(todo.createdAt) }}
                    </span>
                    <span v-if="todo.category" class="text-xs text-purple-600 bg-purple-50 px-1.5 py-0.5 rounded">
                      {{ todo.category }}
                    </span>
                  </div>
                </div>
                
                <!-- 状态图标 -->
                <div class="flex-shrink-0">
                  <FaIcon 
                    v-if="todo.status === 'pending'"
                    name="i-material-symbols:schedule-outline" 
                    class="size-4 text-orange-500" 
                    title="待处理"
                  />
                  <FaIcon 
                    v-else-if="todo.status === 'in_progress'"
                    name="i-material-symbols:play-circle-outline" 
                    class="size-4 text-blue-500" 
                    title="进行中"
                  />
                </div>
              </div>
            </div>
          </div>
        </div>

        <!-- 底部操作 -->
        <div class="p-4 border-t border-gray-200 bg-gray-50 rounded-b-lg">
          <FaButton
            variant="outline"
            class="w-full text-blue-600 border-blue-200 hover:bg-blue-50"
            @click="goToTodoPage"
          >
            <FaIcon name="i-material-symbols:list-outline" class="mr-2 size-4" />
            查看全部待办
          </FaButton>
        </div>
      </div>
    </Transition>
  </div>
</template>

<style scoped>
.line-clamp-2 {
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
}
</style>