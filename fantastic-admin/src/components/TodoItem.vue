<script setup lang="ts">
import type { TodoItem } from '@/utils/websocket'
import { useWebSocket, TodoStatus, TodoPriority } from '@/utils/websocket'

interface Props {
  todo: TodoItem
  showActions?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  showActions: true
})

const { completeTodo, deleteTodo } = useWebSocket()

// 优先级颜色映射
const priorityColors = {
  [TodoPriority.HIGH]: 'bg-red-100 text-red-800 border-red-200',
  [TodoPriority.MEDIUM]: 'bg-yellow-100 text-yellow-800 border-yellow-200',
  [TodoPriority.LOW]: 'bg-green-100 text-green-800 border-green-200'
}

// 状态颜色映射
const statusColors = {
  [TodoStatus.PENDING]: 'bg-gray-100 text-gray-800 border-gray-200',
  [TodoStatus.IN_PROGRESS]: 'bg-blue-100 text-blue-800 border-blue-200',
  [TodoStatus.COMPLETED]: 'bg-green-100 text-green-800 border-green-200'
}

// 格式化日期
function formatDate(dateString: string) {
  const date = new Date(dateString)
  return date.toLocaleDateString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit'
  })
}

// 检查是否过期
function isOverdue(dueDate?: string) {
  if (!dueDate) return false
  return new Date(dueDate) < new Date()
}

// 处理完成待办
function handleComplete() {
  completeTodo(props.todo.id)
}

// 处理删除待办
function handleDelete() {
  deleteTodo(props.todo.id)
}
</script>

<template>
  <div class="bg-white rounded-lg border border-gray-200 p-4 shadow-sm hover:shadow-md transition-shadow">
    <!-- 头部：标题和优先级 -->
    <div class="flex items-start justify-between mb-3">
      <h3 class="text-lg font-semibold text-gray-900 flex-1 mr-3">
        {{ todo.orderType }}#{{ todo.id }}({{ todo.modelName }}){{ todo.category }}
      </h3>
      <div class="flex items-center gap-2">
        <!-- 优先级标签 -->
        <span 
          class="px-2 py-1 text-xs font-medium rounded-full border"
          :class="priorityColors[todo.priority]"
        >
          {{ todo.priority === TodoPriority.HIGH ? '高' : todo.priority === TodoPriority.MEDIUM ? '中' : '低' }}
        </span>
        <!-- 状态标签 -->
        <span 
          class="px-2 py-1 text-xs font-medium rounded-full border"
          :class="statusColors[todo.status]"
        >
          {{ todo.status === TodoStatus.PENDING ? '待处理' : todo.status === TodoStatus.IN_PROGRESS ? '进行中' : '已完成' }}
        </span>
      </div>
    </div>

    <!-- 标题 -->
    <p class="text-gray-600 text-sm mb-3 leading-relaxed">
      {{ todo.title }}
    </p>

    <!-- 分类 -->
    <div v-if="todo.category" class="mb-3">
      <span class="inline-flex items-center px-2 py-1 text-xs font-medium bg-purple-100 text-purple-800 rounded-full border border-purple-200">
        <FaIcon name="i-material-symbols:category-outline" class="mr-1 size-3" />
        {{ todo.category }}
      </span>
    </div>

    <!-- 时间信息 -->
    <div class="flex items-center justify-between text-xs text-gray-500 mb-3">
      <div class="flex items-center gap-4">
        <span class="flex items-center">
          <FaIcon name="i-material-symbols:schedule-outline" class="mr-1 size-3" />
          创建：{{ formatDate(todo.createdAt) }}
        </span>
        <span v-if="todo.dueDate" class="flex items-center" :class="{ 'text-red-500 font-medium': isOverdue(todo.dueDate) }">
          <FaIcon name="i-material-symbols:event-outline" class="mr-1 size-3" />
          截止：{{ formatDate(todo.dueDate) }}
          <FaIcon v-if="isOverdue(todo.dueDate)" name="i-material-symbols:warning-outline" class="ml-1 size-3 text-red-500" />
        </span>
      </div>
    </div>

    <!-- 操作按钮 -->
    <div v-if="showActions && todo.status !== TodoStatus.COMPLETED" class="flex items-center justify-end gap-2 pt-3 border-t border-gray-100">
      <FaButton
        size="sm"
        variant="outline"
        class="flex-1 text-green-600 border-green-200 hover:bg-green-50"
        @click="handleComplete"
      >
        <FaIcon name="i-material-symbols:check-circle-outline" class="mr-1 size-4" />
        完成
      </FaButton>
      <FaButton
        size="sm"
        variant="outline"
        class="flex-1 text-red-600 border-red-200 hover:bg-red-50"
        @click="handleDelete"
      >
        <FaIcon name="i-material-symbols:delete-outline" class="mr-1 size-4" />
        删除
      </FaButton>
    </div>

    <!-- 已完成状态显示 -->
    <div v-if="todo.status === TodoStatus.COMPLETED" class="flex items-center justify-center pt-3 border-t border-gray-100">
      <span class="flex items-center text-green-600 text-sm font-medium">
        <FaIcon name="i-material-symbols:check-circle" class="mr-1 size-4" />
        已完成
      </span>
    </div>
  </div>
</template>