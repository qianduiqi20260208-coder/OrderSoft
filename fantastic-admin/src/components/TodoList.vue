<script setup lang="ts">
import type { TodoItem } from '@/utils/websocket'
import { useWebSocket, TodoStatus } from '@/utils/websocket'
import TodoItemComponent from './TodoItem.vue'

interface Props {
  filter?: TodoStatus
  limit?: number
  showHeader?: boolean
  showActions?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  filter: TodoStatus.ALL,
  limit: 0,
  showHeader: true,
  showActions: true
})

const { todos, unreadCount, markAsRead } = useWebSocket()

// 过滤后的待办事项
const filteredTodos = computed(() => {
  let filtered = todos
  
  if (props.filter !== TodoStatus.ALL) {
    filtered = todos.filter(todo => todo.status === props.filter)
  }
  
  // 按创建时间倒序排列
  filtered = filtered.sort((a, b) => new Date(b.createdAt).getTime() - new Date(a.createdAt).getTime())
  
  // 限制数量
  if (props.limit > 0) {
    filtered = filtered.slice(0, props.limit)
  }
  
  return filtered
})

// 统计信息
const stats = computed(() => {
  const total = todos.length
  const pending = todos.filter(t => t.status === TodoStatus.PENDING).length
  const inProgress = todos.filter(t => t.status === TodoStatus.IN_PROGRESS).length
  const completed = todos.filter(t => t.status === TodoStatus.COMPLETED).length
  
  return { total, pending, inProgress, completed }
})

// 过滤器选项
const filterOptions = [
  { value: TodoStatus.ALL, label: '全部', count: computed(() => stats.value.total) },
  { value: TodoStatus.PENDING, label: '待处理', count: computed(() => stats.value.pending) },
  { value: TodoStatus.IN_PROGRESS, label: '进行中', count: computed(() => stats.value.inProgress) },
  { value: TodoStatus.COMPLETED, label: '已完成', count: computed(() => stats.value.completed) }
]

// 当前过滤器
const currentFilter = ref<TodoStatus>(props.filter)

// 监听过滤器变化
watch(() => props.filter, (newFilter) => {
  currentFilter.value = newFilter
})

// 标记所有为已读
function markAllAsRead() {
  markAsRead()
}

// 获取过滤器标签颜色
function getFilterColor(filter: TodoStatus) {
  switch (filter) {
    case TodoStatus.PENDING:
      return 'text-orange-600 bg-orange-50 border-orange-200'
    case TodoStatus.IN_PROGRESS:
      return 'text-blue-600 bg-blue-50 border-blue-200'
    case TodoStatus.COMPLETED:
      return 'text-green-600 bg-green-50 border-green-200'
    default:
      return 'text-gray-600 bg-gray-50 border-gray-200'
  }
}
</script>

<template>
  <div class="bg-white rounded-lg border border-gray-200 shadow-sm">
    <!-- 头部 -->
    <div v-if="showHeader" class="p-4 border-b border-gray-200">
      <div class="flex items-center justify-between mb-4">
        <h2 class="text-xl font-semibold text-gray-900 flex items-center">
          <FaIcon name="i-material-symbols:task-outline" class="mr-2 size-6" />
          我的待办
          <span v-if="unreadCount > 0" class="ml-2 px-2 py-1 text-xs font-medium bg-red-100 text-red-800 rounded-full">
            {{ unreadCount }} 条新消息
          </span>
        </h2>
        <div class="flex items-center gap-2">
          <FaButton
            v-if="unreadCount > 0"
            size="sm"
            variant="outline"
            class="text-blue-600 border-blue-200 hover:bg-blue-50"
            @click="markAllAsRead"
          >
            <FaIcon name="i-material-symbols:mark-email-read-outline" class="mr-1 size-4" />
            全部已读
          </FaButton>
        </div>
      </div>

      <!-- 过滤器标签 -->
      <div class="flex items-center gap-2 flex-wrap">
        <button
          v-for="option in filterOptions"
          :key="option.value"
          class="px-3 py-1.5 text-sm font-medium rounded-lg border transition-colors"
          :class="[
            currentFilter === option.value
              ? getFilterColor(option.value)
              : 'text-gray-500 bg-white border-gray-200 hover:bg-gray-50'
          ]"
          @click="currentFilter = option.value"
        >
          {{ option.label }}
          <span class="ml-1 px-1.5 py-0.5 text-xs bg-white rounded-full">
            {{ option.count.value }}
          </span>
        </button>
      </div>
    </div>

    <!-- 待办事项列表 -->
    <div class="p-4">
      <div v-if="filteredTodos.length === 0" class="text-center py-12">
        <FaIcon name="i-material-symbols:task-outline" class="mx-auto size-12 text-gray-300 mb-4" />
        <h3 class="text-lg font-medium text-gray-900 mb-2">暂无待办事项</h3>
        <p class="text-gray-500">
          {{ currentFilter === TodoStatus.ALL ? '还没有任何待办事项' : `没有${filterOptions.find(o => o.value === currentFilter)?.label}的待办事项` }}
        </p>
      </div>

      <div v-else class="space-y-4">
        <TodoItemComponent
          v-for="todo in filteredTodos"
          :key="todo.id"
          :todo="todo"
          :show-actions="showActions"
        />
      </div>
    </div>

    <!-- 底部统计 -->
    <div v-if="showHeader && stats.total > 0" class="px-4 py-3 bg-gray-50 border-t border-gray-200 rounded-b-lg">
      <div class="flex items-center justify-between text-sm text-gray-600">
        <span>共 {{ stats.total }} 项待办</span>
        <div class="flex items-center gap-4">
          <span class="flex items-center">
            <div class="w-2 h-2 bg-orange-400 rounded-full mr-1" />
            待处理 {{ stats.pending }}
          </span>
          <span class="flex items-center">
            <div class="w-2 h-2 bg-blue-400 rounded-full mr-1" />
            进行中 {{ stats.inProgress }}
          </span>
          <span class="flex items-center">
            <div class="w-2 h-2 bg-green-400 rounded-full mr-1" />
            已完成 {{ stats.completed }}
          </span>
        </div>
      </div>
    </div>
  </div>
</template>