<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed, nextTick } from 'vue'
import { useRouter } from 'vue-router'
import { useWebSocket } from '@/utils/websocket'
import { formatRelativeTime } from '@/composables/useNotification'
import { useNotification } from '@/composables/useNotification'

const { isConnected, unreadCount, todos, markAsRead } = useWebSocket()
const router = useRouter()
const { toggleToastNotifications } = useNotification()
// 下拉菜单状态
const isDropdownOpen = ref(false)
const processedTodoIds = ref<Set<string>>(new Set())

// 右键菜单状态
const isContextMenuOpen = ref(false)
const contextMenuPosition = ref({ x: 0, y: 0 })

// 通知显示设置
const showNotifications = ref(true)
const showBadgeCount = ref(true)

// 从localStorage加载设置
function loadSettings() {
  const savedNotifications = localStorage.getItem('notification-show-toast')
  const savedBadgeCount = localStorage.getItem('notification-show-badge-count')
  
  if (savedNotifications !== null) {
    showNotifications.value = savedNotifications === 'true'
  }
  if (savedBadgeCount !== null) {
    showBadgeCount.value = savedBadgeCount === 'true'
  }
}

// 保存设置到localStorage
function saveSettings() {
  localStorage.setItem('notification-show-toast', showNotifications.value.toString())
  toggleToastNotifications()  
  localStorage.setItem('notification-show-badge-count', showBadgeCount.value.toString())
}

// 最近的待办事项（最多显示5个）
const recentTodos = computed(() => {
  return todos
    .filter(todo => todo.status !== 'completed')
    .sort((a, b) => new Date(b.createdAt).getTime() - new Date(a.createdAt).getTime())
    // .slice(0, 5)
})

// 点击通知图标
function handleNotificationClick() {
  isDropdownOpen.value = !isDropdownOpen.value
  // 不再在这里标记为已读
}

// 关闭下拉菜单
function closeDropdown() {
  isDropdownOpen.value = false
}

// 跳转到首页
function goToTodoPage() {
  closeDropdown()
  // 跳转到首页
  router.push('/order_manage')
}

// 点击待办事项列表项
function handleTodoItemClick(todoId: string) {
  // 如果该待办事项已经处理过，则不再重复处理
  if (processedTodoIds.value.has(todoId)) {
    return
  }
  
  // 添加到已处理集合中
  processedTodoIds.value.add(todoId)
  
  // 标记单个待办事项为已读，未读数量减一
  markAsRead(todoId)
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
  loadSettings()
  document.addEventListener('click', handleClickOutside)
  document.addEventListener('contextmenu', handleContextMenuOutside)
  document.addEventListener('click', handleContextMenuOutside)
})

onUnmounted(() => {
  document.removeEventListener('click', handleClickOutside)
  document.removeEventListener('contextmenu', handleContextMenuOutside)
  document.removeEventListener('click', handleContextMenuOutside)
})

// 右键点击铃铛图标
function handleContextMenu(event: MouseEvent) {
  event.preventDefault()
  event.stopPropagation()
  
  // 确保菜单显示
  isContextMenuOpen.value = true
  contextMenuPosition.value = {
    x: event.screenX,
    y: 0 // 取绝对值
  }
  
  // 强制更新DOM
  nextTick(() => {
    console.log('菜单状态:', isContextMenuOpen.value, '位置:', contextMenuPosition.value)
  })
}

// 点击外部关闭右键菜单
function handleContextMenuOutside(event: Event) {
  const target = event.target as HTMLElement
  const contextMenu = document.querySelector('.context-menu')
  const button = document.querySelector('.notification-button')
  
  if (contextMenu && !contextMenu.contains(target) && button && !button.contains(target)) {
    isContextMenuOpen.value = false
  }
  
  // 如果点击的是通知按钮本身，不关闭菜单（因为按钮有自己的处理逻辑）
  if (button && button.contains(target)) {
    return
  }
}

// 切换通知显示
function toggleNotifications() {
  showNotifications.value = !showNotifications.value
  isContextMenuOpen.value = false
  saveSettings()
}

// 切换徽章数量显示
function toggleBadgeCount() {
  showBadgeCount.value = !showBadgeCount.value
  isContextMenuOpen.value = false
  saveSettings()
}


// 开发模式判断
const isDevMode = import.meta.env.DEV
</script>

<template>
  <div class="relative">
    <!-- 通知图标按钮 -->
    <FaButton variant="ghost" size="icon" class="notification-button relative size-9" 
      @click="handleNotificationClick"
      @contextmenu.prevent="handleContextMenu">
      <!-- 连接状态指示器 -->
      <div v-if="isDevMode" class="absolute -top-1 -left-1 w-3 h-3 rounded-full border-2 border-white"
        :class="isConnected ? 'bg-green-400' : 'bg-red-400'" :title="isConnected ? 'WebSocket已连接' : 'WebSocket未连接'" />

      <!-- 通知图标 -->
      <FaIcon :name="showNotifications ? 'i-material-symbols:notifications-outline' : 'i-material-symbols:notifications-off-outline'" class="size-5"
        :class="unreadCount > 0 && showBadgeCount && showNotifications ? 'text-blue-600' : 'text-gray-600'" />

      <!-- 未读消息数量徽章 -->
      <div v-if="unreadCount > 0 && showBadgeCount"
        class="absolute -top-2 -right-2 min-w-5 h-5 bg-gradient-to-br from-red-400 to-red-600 text-white text-xs font-bold rounded-full flex items-center justify-center px-1 border-2 border-white/90 backdrop-blur-sm shadow-lg">
        {{ unreadCount > 99 ? '99+' : unreadCount }}
      </div>
    </FaButton>

    <!-- 右键菜单 -->
    <div v-if="isContextMenuOpen" 
      class="context-menu fixed bg-white rounded-lg shadow-lg border border-gray-200 py-2 z-[9999] min-w-[150px]"
      :style="{ left: contextMenuPosition.x + 'px', top: contextMenuPosition.y + 'px' }"
      @click.stop>
      <button @click="toggleBadgeCount" class="w-full px-4 py-2 text-left text-sm hover:bg-gray-100 flex items-center gap-2">
        <FaIcon :name="showBadgeCount ? 'i-material-symbols:numbers' : 'i-material-symbols:numbers-off'" class="size-4" />
        {{ showBadgeCount ? '隐藏数量' : '显示数量' }}
      </button>
      <button @click="toggleNotifications" class="w-full px-4 py-2 text-left text-sm hover:bg-gray-100 flex items-center gap-2">
        <FaIcon :name="showNotifications ? 'i-material-symbols:visibility' : 'i-material-symbols:visibility-off'" class="size-4" />
        {{ showNotifications ? '隐藏通知' : '显示通知' }}
      </button>
    </div>

    <!-- 下拉菜单 -->
    <Transition enter-active-class="transition ease-out duration-200" enter-from-class="opacity-0 scale-95"
      enter-to-class="opacity-100 scale-100" leave-active-class="transition ease-in duration-150"
      leave-from-class="opacity-100 scale-100" leave-to-class="opacity-0 scale-95">
      <div v-if="isDropdownOpen"
        class="notification-dropdown absolute right-0 top-full mt-2 w-96 bg-white rounded-lg shadow-lg border border-gray-200 z-50">
        <!-- 头部 -->
        <div class="p-4 border-b border-gray-200">
          <div class="flex items-center justify-between">
            <h3 class="text-lg font-semibold text-gray-900 flex items-center">
              <FaIcon :name="showNotifications ? 'i-material-symbols:notifications-outline' : 'i-material-symbols:notifications-off-outline'" class="mr-2 size-5" />
              通知中心
            </h3>
            <div class="flex items-center gap-2">
              <!-- 连接状态 开发模式才显示 -->
              <div v-if="isDevMode" class="flex items-center text-xs">
                <div class="w-2 h-2 rounded-full mr-1" :class="isConnected ? 'bg-green-400' : 'bg-red-400'" />
                <span :class="isConnected ? 'text-green-600' : 'text-red-600'">
                  {{ isConnected ? '已连接' : '未连接' }}
                </span>
              </div>
              <FaButton variant="ghost" size="icon" class="size-6" @click="closeDropdown">
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
            <div v-for="todo in recentTodos" :key="todo.id"
              class="p-4 hover:bg-gray-50 transition-colors cursor-pointer"
              @click="handleTodoItemClick(todo.id)">
              <div class="flex items-start gap-3">
                <!-- 优先级指示器 -->
                <div class="w-2 h-2 rounded-full mt-2 flex-shrink-0" :class="{
                  'bg-red-400': todo.priority === 'high',
                  'bg-yellow-400': todo.priority === 'medium',
                  'bg-green-400': todo.priority === 'low'
                }" />

                <div class="flex-1 min-w-0">
                  <!-- 标题和时间 -->
                  <div class="flex items-center gap-2 mb-1">
                    <!-- <FaIcon name="i-material-symbols:schedule-outline" class="size-3 text-gray-400" /> -->
                    <h4 class="text-sm font-medium text-gray-900 truncate">
                      {{ todo.title }}
                    </h4>
                  </div>
                  <span class="text-sm text-gray-600 p-1 break-words">
                    <span :class="{
                      'text-blue-700': todo.orderType === '问题复现',
                      'text-green-700': todo.orderType === '版本迭代',
                      'text-yellow-700': todo.orderType === '交付发送',
                      'text-purple-700': todo.orderType === '版本迭代+交付发送',
                      'text-pink-700': todo.orderType === '功能开发',
                      'text-gray-700': todo.orderType === '其他'
                    }">{{ todo.orderType }}</span><span class="text-blue-600 hover:text-blue-800 hover:underline cursor-pointer" @click.stop="handleTodoItemClick(todo.id); router.push('/order_manage')">#{{ todo.id }}</span>({{ todo.modelName }}){{ todo.category }}
                  </span>

                  <!-- 分类标签 -->
                  <div class="flex items-center gap-2">
                    <span class="text-xs text-purple-600 bg-purple-50 px-1.5 py-0.5 rounded">
                      {{ todo.category }}
                    </span>
                  </div>
                </div>
                <!-- 状态图标 -->
                <div class="flex-shrink-0 flex items-center gap-1">
                  <span class="text-xs text-gray-500">
                    {{ formatRelativeTime(todo.createdAt) }}
                  </span>
                  <FaIcon v-if="todo.status === 'pending'" name="i-material-symbols:schedule-outline"
                    class="size-4 text-orange-500" title="待处理" />
                  <FaIcon v-else-if="todo.status === 'in_progress'" name="i-material-symbols:play-circle-outline"
                    class="size-4 text-blue-500" title="进行中" />
                </div>
              </div>
            </div>
          </div>
        </div>

        <!-- 底部操作 -->
        <div class="p-4 border-t border-gray-200 bg-gray-50 rounded-b-lg">
          <FaButton variant="outline" class="w-full text-blue-600 border-blue-200 hover:bg-blue-50"
            @click="goToTodoPage">
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