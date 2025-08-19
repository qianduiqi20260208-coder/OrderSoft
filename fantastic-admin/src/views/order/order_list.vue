<route lang="yaml">
meta:
  title: 工单列表
  # enabled: false
</route>

<script setup lang="ts">
import { ElMessage } from 'element-plus'
import { onMounted, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import orderApi from '@/api/modules/order'
import { useTabbarStore } from '@/store/modules/tabbar'
import { useUserStore } from '@/store/modules/user'

// -----------------数据结构-----------------
// 添加路由实例
const route = useRoute()
const router = useRouter()
const tabbar = useTabbarStore() // 添加 tabbar 实例
const userStore = useUserStore() // 添加用户store实例

// 修改：从userStorage加载模型列表
const modelList = ref<Array<{ id: string, name: string }>>([])
// 检查是否从发送详情页面跳转而来
const isFromSendDetail = ref(false)
// 流转相关数据结构
interface TransferInfo {
  transferExecutorID: string
  transferReason: string
  transferTime: string
}
// 定义工单所包括的内容
interface OrderItem {
  // 工单类型
  type: '问题复现' | '版本迭代' | '交付发送' | '版本迭代+交付发送' | '功能开发' | '其他'
  // 工单状态
  status: '草稿' | '待审批' | '待分发' | '进行中' | '已完成' | '已退回'
  // 参考优先级
  referencePriority?: '紧急' | '一般' | ''
  // 任务优先级
  taskPriority?: '紧急' | '一般' | ''

  orderID: string // 工单ID
  modelID: string // 模型ID
  modelVersionID: string // 模型版本ID
  promoterID: string // 发起人ID
  startTime: string // 发起时间
  approverID?: string // 审批人ID
  distributorID?: string // 分发人ID
  approveTime?: string // 审批时间
  distributeTime?: string // 分发时间
  rejectReason?: string // 拒绝原因
  executorID?: string // 执行人ID
  finishTime?: string // 完成时间

  // ----------问题复现工单（创建）----------
  coordinationID?: string // 协调单号
  description?: string // 复现内容描述
  // 后端返回的文件相关字段
  fileName?: string // 文件名
  fileUrl?: string // 文件URL
  hasAttachment?: boolean // 是否有附件
  // 前端使用的文件数组（转换后的格式）
  files?: Array<{ fileName: string, fileUrl: string }> // 附件列表

  // ----------版本迭代类（创建）----------
  updateNotes?: string // 更新内容
  packageRequirement?: string // 封装要求
  apiChanged?: string // 接口是否变化（是/否）

  // ----------交付发送类（创建）----------
  targetCustomer?: string // 目标客户）
  isCAEChecked?: string // 是否通过CAE平台验证（是/否）
  hasSensitiveInfo?: string // 当前版本是否包含敏感信息（是/否）

  // ----------功能开发类（创建）----------
  featureDesc?: string // 功能描述

  // ----------其他类（创建）----------
  contentDesc?: string // 内容描述

  // ----------完成工单相关字段（不可与创建复用）----------
  finishRemark?: string // 备注
  // 问题复现类（完成）
  finishPhenomenon?: string // 复现现象

  // 版本迭代类（完成）
  finishModelVersion?: string // 升级后模型版本

  // 交付发送类（完成）
  isEncrypted?: string // 是否加密（是/否）
  finishAuthId?: string // 授权ID（数字）
  finishShellNo?: string // 外壳号（字母+数字）

  // 功能开发类（完成）
  finishModelVersionId?: string // 完成后模型版本ID
  finishFeatureDesc?: string // 完成功能描述

  // 其他类（完成）
  finishRemarkOther?: string // 备注（完成）

  transfers?: TransferInfo[] // 多次流转记录
}

// 工单列表分页相关变量
const userOrders = ref<OrderItem[]>([]) // 当前页工单数据
const loading = ref(false) // 加载状态
const pageSize = 10 // 每页数量
const currentPage = ref(1) // 当前页码
const total = ref(0) // 总工单数

// 筛选相关变量
const filterOrderID = ref('') // 工单ID搜索框
const filterType = ref('') // 工单类型下拉框
const filterPromoterID = ref('') // 员工工号下拉框
const filterDateRange = ref<[string, string]>(['', '']) // 日期区间选择器
const filterModelID = ref('') // 模型下拉框
const filterReferencePriority = ref('') // 参考优先级下拉框
const filterTaskPriority = ref('') // 任务优先级下拉框
const filterStatus = ref('') // 工单状态下拉框

// 新增：每个工单的展开状态
const expandedMap = ref<Record<string, boolean>>({})

// 详细信息弹窗功能
const dialogVisible = ref(false)
const currentOrder = ref<OrderItem | null>(null)

// ----------事件处理函数----------
// 文件下载功能
async function downloadFile(fileUrl: string, fileName: string) {
  try {
    // 从fileUrl中提取ticketId
    const urlParts = fileUrl.split('/')
    const ticketIdIndex = urlParts.findIndex(part => part === 'ticket')

    if (ticketIdIndex === -1 || ticketIdIndex + 1 >= urlParts.length) {
      throw new Error('无法从文件URL中提取工单ID')
    }

    const ticketId = Number.parseInt(urlParts[ticketIdIndex + 1])

    if (Number.isNaN(ticketId)) {
      throw new TypeError('工单ID格式不正确')
    }

    // 调用后端API下载文件
    const response = await orderApi.downloadTicketFile(ticketId, fileName)

    // 从响应数据中提取文件信息
    const { content, contentType, size } = response.data

    // 将Base64内容转换为二进制数据
    const binaryString = window.atob(content)
    const bytes = new Uint8Array(binaryString.length)
    for (let i = 0; i < binaryString.length; i++) {
      bytes[i] = binaryString.charCodeAt(i)
    }

    // 创建blob对象
    const blob = new Blob([bytes], {
      type: contentType || 'application/octet-stream',
    })

    // 验证文件大小（可选）
    if (blob.size !== size) {
      console.warn(`文件大小不匹配: 期望 ${size} bytes, 实际 ${blob.size} bytes`)
    }

    // 创建下载URL
    const url = window.URL.createObjectURL(blob)

    // 创建下载链接
    const link = document.createElement('a')
    link.href = url
    link.download = fileName
    document.body.appendChild(link) // 确保添加到DOM
    link.click()
    document.body.removeChild(link) // 移除

    // 清理URL
    window.URL.revokeObjectURL(url)

    ElMessage.success('文件下载成功')
  }
  catch (error) {
    ElMessage.error('文件下载失败')
    console.error('下载文件失败:', error)
  }
}

// 修改原有的分页查询函数，添加来源标识
async function fetchUserOrders(page = 1) {
  loading.value = true
  try {
    // 原有的分页查询逻辑
    const params: {
      page: number
      pageSize: number
      orderID?: string
      type?: string
      promoterID?: string
      modelID?: string
      referencePriority?: string
      taskPriority?: string
      status?: string
      startDate?: string
      endDate?: string
    } = {
      page,
      pageSize,
      orderID: filterOrderID.value,
      type: filterType.value,
      promoterID: filterPromoterID.value,
      modelID: filterModelID.value,
      referencePriority: filterReferencePriority.value,
      taskPriority: filterTaskPriority.value,
      status: filterStatus.value,
    }

    if (filterDateRange.value) {
      params.startDate = filterDateRange.value[0]
      params.endDate = filterDateRange.value[1]
    }

    // 查询前做限制 模型工程师只能查看自己负责的模型
    if (Array.isArray(userStore.permissions) && userStore.permissions.includes('ModelEngineer')) {
      if (!filterModelID.value || filterModelID.value === '') {
        // 只查自己负责的模型
        params.modelID = userStore.userModels.join(',')
      }
    }

    const res = await orderApi.fetchOrderPage(params)
    // 处理返回的数据，将单个文件转换为文件数组
    const orders = res.data.list || []
    userOrders.value = orders.map((order: any) => {
      // 如果有文件信息，转换为 files 数组格式
      if (order.hasAttachment && order.fileName && order.fileUrl) {
        order.files = [{
          fileName: order.fileName,
          fileUrl: order.fileUrl,
        }]
      }
      else {
        order.files = []
      }

      return order
    })

    total.value = res.data.total || 0
    currentPage.value = res.data.page || 1

    console.warn('工单列表查询成功:', { page, total: total.value })
  }
  finally {
    loading.value = false
  }
}

// 修改搜索按钮点击事件
function handleSearch() {
  // 重置展开状态
  expandedMap.value = {}
  fetchUserOrders(1)
}

// 修改分页组件页码变化事件
function handlePageChange(page: number) {
  fetchUserOrders(page)
}

// 加载用户负责的模型列表
function loadUserModels() {
  try {
    // 从userStorage获取用户负责的模型
    const userModels = userStore.userModels || []

    if (userModels.length > 0) {
      // 转换为下拉框需要的格式
      modelList.value = userModels.map(model => ({
        id: model,
        name: model,
      }))
    }
    else {
      ElMessage.warning('userStorage中无模型数据，使用默认模型列表')
    }
  }
  catch (error) {
    console.error('加载用户模型失败:', error)

    ElMessage.warning('加载用户模型列表失败')
  }
}

// 修改页面加载逻辑
onMounted(() => {
  // 加载用户负责的模型
  loadUserModels()

  // 检查是否从发送详情页面跳转而来
  const orderNumber = route.query.orderId as string
  const fromSendDetail = route.query.fromSendDetail === 'true'

  // 设置标识状态
  isFromSendDetail.value = fromSendDetail

  // 新增：如果当前用户是ModelEngineer，只能看自己负责的模型
  const permissions = userStore.permissions || []
  if (Array.isArray(permissions) && permissions.includes('ModelEngineer')) {
    // 只显示用户负责的模型
    const userModels = userStore.userModels || []
    // 如果有模型，默认筛选第一个模型（或全部模型）
    if (userModels.length > 0) {
      // 这里可以让用户选择，也可以直接用全部模型
      filterModelID.value = userModels[0]
    }
  }

  if (orderNumber && fromSendDetail) {
    // 从发送详情页面跳转，设置筛选条件并执行筛选
    filterOrderID.value = orderNumber
    fetchUserOrders(1)
  }
  else {
    // 正常页面加载，查询第一页
    fetchUserOrders(1)
  }
})

// 展开/收起指定工单内容块
function expandOrder(orderId: string, expand: boolean) {
  expandedMap.value[orderId] = expand
}

// 显示工单详细信息弹窗
function showOrderDetail(order: OrderItem) {
  currentOrder.value = order
  dialogVisible.value = true
}

// 关闭工单详细信息弹窗
function closeDialog() {
  dialogVisible.value = false
  currentOrder.value = null
}

// --------------员工工号下拉------------------
// 修改：用于员工工号下拉（从后端获取）
const userList = ref<Array<{ id: string, name: string }>>([])
const userListLoading = ref(false) // 执行人列表加载状态

// 获取执行人列表
async function fetchExecutorList() {
  // 如果已经有数据，避免重复请求
  if (userList.value.length > 0) {
    return
  }

  userListLoading.value = true

  try {
    const res = await orderApi.fetchAllExecutorList()

    if (res?.data) {
      // 手动数据映射处理，适配新的响应格式
      const executorList = res.data.list || []

      const mappedUserList = executorList.map((executorId: number | string) => ({
        id: String(executorId), // 确保ID是字符串格式
        name: String(executorId), // 如果没有姓名，使用ID作为显示名称
      }))

      userList.value = mappedUserList
    }
    else {
      ElMessage.error('获取执行人列表失败：响应数据为空')
      userList.value = []
    }
  }
  catch (error: any) {
    console.error('获取执行人列表失败:', error)

    // 处理不同类型的错误
    if (error?.response?.data?.message) {
      ElMessage.error(error.response.data.message)
    }
    else if (error?.message) {
      ElMessage.error(error.message)
    }
    else {
      ElMessage.error('获取执行人列表失败，请稍后重试')
    }
  }
  finally {
    userListLoading.value = false
  }
}

// 员工工号下拉框显示事件处理
function handleExecutorDropdownVisible(visible: boolean) {
  if (visible) {
    // 下拉框打开时获取执行人列表
    fetchExecutorList()
  }
}

// 返回发送详情页面
function handleBackToSendDetail() {
  // 获取当前标签页的完整路径
  const currentTabPath = route.fullPath

  // 先执行返回操作
  router.back()

  // 延迟关闭当前标签页，确保路由跳转完成
  setTimeout(() => {
    tabbar.remove(currentTabPath)
  }, 100)
}
</script>

<template>
  <div>
    <!-- 顶部标题栏 -->
    <div class="mb-6 border border-gray-200 rounded-lg bg-white p-6 shadow-sm">
      <div class="flex items-center justify-between">
        <div class="flex items-center">
          <h1 class="text-2xl text-gray-800 font-bold">
            工单列表
          </h1>
        </div>
      </div>

      <!-- 文字提示行 -->
      <div class="mt-3 text-sm text-gray-600">
        本页面支持展示所有工单，可以通过筛选条件查找特定工单，点击“详细信息”按钮查看工单的完整信息。
      </div>
    </div>
    <!-- 页面顶部内容块，添加筛选功能示例 -->
    <FaPageMain>
      <template #title>
        <div class="flex-center-between">
          <span class="text-2xl text-black font-bold">工单筛选</span>
          <!-- 如果是从发送详情页面跳转来的，显示返回按钮 -->
          <div v-if="isFromSendDetail" class="flex items-center">
            <FaButton
              class="border border-gray-500 rounded bg-gray-300 px-5 py-2 text-black font-semibold transition-colors duration-150 hover:bg-gray-400"
              @click="handleBackToSendDetail"
            >
              <i class="i-mdi-arrow-left mr-2" />
              返回发送详情
            </FaButton>
          </div>
        </div>
      </template>
      <div class="py-2 space-y-4">
        <!-- 搜索选项占满一行 -->
        <div class="w-full flex items-center gap-4">
          <!-- 工单ID搜索框 -->
          <el-input
            v-model="filterOrderID"
            placeholder="工单ID（如 GX-250701-001）"
            clearable
            class="min-w-[240px] flex-1"
            @keyup.enter="handleSearch"
          />

          <!-- 工单类型下拉框 -->
          <el-select v-model="filterType" placeholder="工单类型" clearable class="min-w-[120px] flex-1">
            <el-option label="全部" value="" />
            <el-option label="GX(更新)" value="版本迭代" />
            <el-option label="JF(交付)" value="交付发送" />
            <el-option label="GJ(更新交付)" value="直接封装+发送" />
            <el-option label="FX(复现)" value="问题复现" />
            <el-option label="KF(开发)" value="功能开发" />
            <el-option label="QT(其他)" value="其他" />
          </el-select>

          <!-- 员工工号下拉框 - 只显示工号 -->
          <el-select
            v-model="filterPromoterID"
            placeholder="发起员工工号"
            clearable
            class="min-w-[120px] flex-1"
            :loading="userListLoading"
            @visible-change="handleExecutorDropdownVisible"
          >
            <el-option label="全部" value="" />
            <el-option
              v-for="user in userList"
              :key="user.id"
              :label="user.id"
              :value="user.id"
            >
              <div class="flex items-center">
                <span>{{ user.id }}</span>
              </div>
            </el-option>
          </el-select>

          <!-- 模型下拉框 -->
          <el-select v-model="filterModelID" placeholder="模型" clearable class="min-w-[120px] flex-1">
            <!-- 只在非ModelEngineer时显示“全部”选项 -->
            <el-option
              v-if="!Array.isArray(userStore.permissions) || !userStore.permissions.includes('ModelEngineer')"
              label="全部"
              value=""
            />
            <el-option
              v-for="model in modelList"
              :key="model.id"
              :label="model.name"
              :value="model.id"
            />
          </el-select>

          <!-- 优先级下拉框（参考优先级） -->
          <el-select v-model="filterReferencePriority" placeholder="参考优先级" clearable class="min-w-[120px] flex-1">
            <el-option label="全部" value="" />
            <el-option label="紧急" value="紧急" />
            <el-option label="一般" value="一般" />
          </el-select>

          <!-- 优先级下拉框（任务优先级） -->
          <el-select v-model="filterTaskPriority" placeholder="任务优先级" clearable class="min-w-[120px] flex-1">
            <el-option label="全部" value="" />
            <el-option label="紧急" value="紧急" />
            <el-option label="一般" value="一般" />
          </el-select>

          <!-- 状态下拉框 -->
          <el-select v-model="filterStatus" placeholder="工单状态" clearable class="min-w-[120px] flex-1">
            <el-option label="全部" value="" />
            <el-option label="草稿" value="草稿" />
            <el-option label="待审批" value="待审批" />
            <el-option label="待分发" value="待分发" />
            <el-option label="进行中" value="进行中" />
            <el-option label="已完成" value="已完成" />
            <el-option label="已退回" value="已退回" />
          </el-select>
        </div>

        <!-- 搜索按钮单独占一行居中 -->
        <div class="w-full flex justify-center">
          <el-button type="primary" size="large" @click="handleSearch">
            <i class="i-mdi-magnify mr-2" />
            搜索
          </el-button>
        </div>
      </div>
    </FaPageMain>

    <el-skeleton :loading="loading" animated>
      <template #default>
        <div class="space-y-4">
          <!-- 每个工单一个内容块 -->
          <FaPageMain
            v-for="order in userOrders"
            :key="order.orderID"
            :title="`工单#${order.orderID}`"
            :collaspe="!expandedMap[order.orderID]"
            height="auto"
            class="w-full"
          >
            <template #title>
              <div class="w-full flex items-center justify-between">
                <div>
                  <span class="block flex items-center text-lg text-black font-extrabold tracking-wide">
                    <!-- 新增：工单前面增加指示 -->
                    <span
                      class="mr-3 inline-block align-middle"
                      style="width: 14px; height: 14px; border-radius: 50%;"
                      :style="{
                        background:
                          order.status === '草稿'
                            ? '#facc15' // 黄色 - 草稿
                            : order.status === '待审批'
                              ? '#f97316' // 橙色 - 待审批
                              : order.status === '待分发'
                                ? '#a855f7' // 紫色 - 待分发
                                : order.status === '进行中'
                                  ? '#3b82f6' // 蓝色 - 进行中
                                  : order.status === '已完成'
                                    ? '#22c55e' // 绿色 - 已完成
                                    : order.status === '已退回'
                                      ? '#ef4444' // 红色 - 已退回
                                      : '#6b7280', // 灰色 - 默认
                      }"
                      :title="order.status"
                    />
                    <!-- 修改：工单类型在前面，加大字号和加粗 -->
                    <span
                      class="mr-4 text-xl font-black"
                      :class="{
                        'text-blue-700': order.type === '问题复现',
                        'text-green-700': order.type === '版本迭代',
                        'text-yellow-700': order.type === '交付发送',
                        'text-purple-700': order.type === '版本迭代+交付发送',
                        'text-pink-700': order.type === '功能开发',
                        'text-gray-700': order.type === '其他',
                      }"
                    >
                      {{ order.type }}
                    </span>

                    <!-- 修改：工单号在后面，相对较小的字号 -->
                    <span class="text-lg text-gray-600 font-bold">
                      工单#{{ order.orderID }}
                    </span>
                    <!-- 详细信息按钮，左侧增加空格 -->
                    <span style="margin-left: 24px;">
                      <el-button type="primary" size="small" @click="showOrderDetail(order)">
                        详细信息
                      </el-button>
                    </span>
                  </span>
                  <div class="mt-2 flex flex-wrap items-center gap-6 text-sm">
                    <!-- 通用展开/收起图标放在发起人ID左边，直接点击图标触发展开/收起 -->
                    <FaIcon
                      name="expand"
                      class="mr-0 cursor-pointer text-xl"
                      @click="expandOrder(order.orderID, !expandedMap[order.orderID])"
                    />
                    <span>
                      <i class="i-mdi-cube mr-1 text-blue-400" />
                      <span class="text-gray-600">模型：</span>
                      <span class="text-black font-bold">{{ order.modelID }}</span>
                    </span>
                    <span>
                      <i class="i-mdi-account mr-1 text-blue-400" />
                      <span class="text-gray-600">发起人ID：</span>
                      <span class="text-black font-bold">{{ order.promoterID }}</span>
                    </span>
                    <span>
                      <i class="i-mdi-calendar-clock mr-1 text-blue-400" />
                      <span class="text-gray-600">发起时间：</span>
                      <span class="text-black font-bold">{{ order.startTime }}</span>
                    </span>
                    <span>
                      <i class="i-mdi-flag mr-1 text-blue-400" />
                      <span class="text-gray-600">参考优先级：</span>
                      <span class="text-black font-bold">{{ order.referencePriority }}</span>
                    </span>
                    <span>
                      <i class="i-mdi-alert mr-1 text-blue-400" />
                      <span class="text-gray-600">任务优先级：</span>
                      <span class="text-black font-bold">{{ order.taskPriority }}</span>
                    </span>
                    <span>
                      <span>
                        <i class="i-mdi-progress-clock mr-1 text-blue-400" />
                        <span class="text-gray-600">当前状态：</span>
                        <span
                          class="ml-2 rounded-full px-3 py-1 font-bold"
                          :class="{
                            'bg-yellow-100 text-yellow-700': order.status === '草稿',
                            'bg-orange-100 text-orange-700': order.status === '待审批',
                            'bg-purple-100 text-purple-700': order.status === '待分发',
                            'bg-blue-100 text-blue-700': order.status === '进行中',
                            'bg-green-100 text-green-700': order.status === '已完成',
                            'bg-red-100 text-red-700': order.status === '已退回',
                          }"
                        >
                          {{ order.status }}
                        </span>
                      </span>
                    </span>
                  </div>
                </div>
              </div>
            </template>
            <div class="mt-4 space-y-3">
              <!-- 完成工单 -->
              <FaPageMain
                v-if="['进行中', '已完成'].includes(order.status)"
                title=""
                :collaspe="!expandedMap[order.orderID]"
                height="auto"
                class="w-full"
              >
                <template #title>
                  <div class="w-full flex items-center justify-between">
                    <div class="flex items-center">
                      <span class="text-lg text-blue-900 font-bold">完成工单</span>
                      <span
                        v-if="order.status === '进行中'"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;"
                        title="进行中"
                      />
                      <span
                        v-else-if="order.status === '已完成'"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;"
                        title="已完成"
                      />
                    </div>
                    <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                      <span>负责人：{{ order.executorID }}</span>
                      <span>完成时间：{{ order.finishTime }}</span>
                    </div>
                  </div>
                </template>
                <div
                  class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded bg-gray-50 px-6 py-4"
                  :class="order.status === '进行中' ? 'bg-gray-50' : 'bg-gray-100 opacity-70'"
                >
                  <!-- 问题复现类工单 -->
                  <template v-if="order.type === '问题复现'">
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">复现现象：</span>
                      <textarea
                        :value="order.finishPhenomenon"
                        class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                        rows="2"
                        readonly
                      />
                    </div>
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">备注：</span>
                      <textarea
                        :value="order.finishRemark"
                        class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                        rows="2"
                        readonly
                      />
                    </div>
                  </template>

                  <!-- 版本迭代类工单 -->
                  <template v-else-if="order.type === '版本迭代'">
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">升级后模型版本：</span>
                      <input
                        :value="order.finishModelVersion"
                        class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                        readonly
                      >
                    </div>
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">备注：</span>
                      <textarea
                        :value="order.finishRemark"
                        class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                        rows="2"
                        readonly
                      />
                    </div>
                  </template>

                  <!-- 交付发送类工单 -->
                  <template v-else-if="order.type === '交付发送'">
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">是否加密：</span>
                      <el-select
                        v-model="order.isEncrypted"
                        placeholder="请选择"
                        class="flex-1"
                        :disabled="order.status === '已完成'"
                      >
                        <el-option label="是" value="是" />
                        <el-option label="否" value="否" />
                      </el-select>
                    </div>
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">授权ID：</span>
                      <input
                        :value="order.finishAuthId"
                        class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                        readonly
                      >
                    </div>
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">外壳号：</span>
                      <input
                        :value="order.finishShellNo"
                        class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                        readonly
                      >
                    </div>
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">备注：</span>
                      <textarea
                        :value="order.finishRemark"
                        class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                        rows="2"
                        readonly
                      />
                    </div>
                  </template>

                  <!-- 版本迭代+交付发送类工单 -->
                  <template v-else-if="order.type === '版本迭代+交付发送'">
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">升级后模型版本：</span>
                      <input
                        :value="order.finishModelVersion"
                        class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                        readonly
                      >
                    </div>
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">是否加密：</span>
                      <el-select
                        v-model="order.isEncrypted"
                        placeholder="请选择"
                        class="flex-1"
                        :disabled="order.status === '已完成'"
                      >
                        <el-option label="是" value="是" />
                        <el-option label="否" value="否" />
                      </el-select>
                    </div>
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">授权ID：</span>
                      <input
                        :value="order.finishAuthId"
                        class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                        readonly
                      >
                    </div>
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">外壳号：</span>
                      <input
                        :value="order.finishShellNo"
                        class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                        readonly
                      >
                    </div>
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">备注：</span>
                      <textarea
                        :value="order.finishRemark"
                        class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                        rows="2"
                        readonly
                      />
                    </div>
                  </template>

                  <!-- 功能开发类工单 -->
                  <template v-else-if="order.type === '功能开发'">
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">升级后模型版本：</span>
                      <input
                        :value="order.finishModelVersionId"
                        class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                        readonly
                      >
                    </div>
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">完成功能描述：</span>
                      <textarea
                        :value="order.finishFeatureDesc"
                        class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                        rows="2"
                        readonly
                      />
                    </div>
                  </template>

                  <!-- 其他类工单 -->
                  <template v-else-if="order.type === '其他'">
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">备注：</span>
                      <textarea
                        :value="order.finishRemarkOther"
                        class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                        rows="2"
                        readonly
                      />
                    </div>
                  </template>
                </div>
              </FaPageMain>
              <!-- 多次流转内容块，循环显示每一次流转（紧跟在任务分发后面），倒序显示 -->
              <template v-if="order.status === '已完成' || order.status === '进行中'">
                <template v-for="(transfer, idx) in (order.transfers ? [...order.transfers].reverse() : [])" :key="idx">
                  <FaPageMain
                    title=""
                    :collaspe="!expandedMap[order.orderID]"
                    height="auto"
                    class="w-full"
                  >
                    <template #title>
                      <div class="w-full flex items-center justify-between">
                        <div class="flex items-center">
                          <span class="text-lg text-blue-900 font-bold">
                            工单流转
                            {{ Array.isArray(order.transfers) && order.transfers.length > 1 ? `（第${order.transfers.length - idx}次）` : '' }}
                          </span>
                          <span
                            class="ml-2 inline-block align-middle"
                            style="width: 12px;height: 12px;background: #22c55e ;border-radius: 50%;"
                            title="流转"
                          />
                        </div>
                        <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                          <span>流转负责人：{{ transfer.transferExecutorID }}</span>
                          <span>流转时间：{{ transfer.transferTime }}</span>
                        </div>
                      </div>
                    </template>
                    <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded bg-gray-50 px-6 py-4">
                      <div class="col-span-2 w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">流转原因：</span>
                        <textarea
                          class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                          :value="transfer.transferReason"
                          rows="2"
                          readonly
                        />
                      </div>
                    </div>
                  </FaPageMain>
                </template>
              </template>
              <!-- 任务分发 -->
              <FaPageMain
                v-if="['待分发', '进行中', '已完成', '已退回'].includes(order.status) && order.distributorID !== '-1'"
                title=""
                :collaspe="!expandedMap[order.orderID]"
                height="auto"
                class="w-full"
              >
                <template #title>
                  <div class="w-full flex items-center justify-between pr-4">
                    <div class="flex items-center">
                      <span class="text-lg text-blue-900 font-bold">任务分发</span>
                      <!-- 只有“待分发”时橙点，“进行中”或“已完成”时绿点 -->
                      <span
                        v-if="order.status === '待分发'"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;"
                        title="进行中"
                      />
                      <span
                        v-else-if="['进行中', '已完成'].includes(order.status)"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;"
                        title="已完成"
                      />
                      <span
                        v-else-if="order.status === '已退回'"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #ef4444;border-radius: 50%;"
                        title="已退回"
                      />
                    </div>
                    <!-- 负责人和完成时间（分发人ID和分发时间） -->
                    <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                      <span>负责人：{{ order.distributorID }}</span>
                      <span>完成时间：{{ order.distributeTime }}</span>
                    </div>
                  </div>
                </template>
                <div
                  class="flex items-center justify-between rounded px-6 py-4"
                  :class="order.status === '待分发' ? 'bg-gray-50' : 'bg-gray-100 opacity-70'"
                >
                  <div class="flex items-center gap-3">
                    <span class="ml-4 text-gray-700 font-semibold">任务优先级：</span>
                    <el-input
                      :model-value="order.taskPriority"
                      size="small"
                      style="width: 120px;"
                      disabled
                    />
                    <!-- 下一流程负责人（执行人ID） -->
                    <span class="ml-4 text-gray-700 font-semibold">下一流程负责人：</span>
                    <el-input
                      :model-value="order.executorID"
                      size="small"
                      style="width: 120px;"
                      disabled
                    />
                    <!-- 新增：已退回时显示拒绝原因 -->
                    <div v-if="order.status === '已退回'" class="flex items-center gap-3">
                      <span class="ml-4 text-red-600 font-semibold">拒绝原因：</span>
                      <el-input
                        :model-value="order.rejectReason"
                        size="small"
                        style="width: 220px;"
                        disabled
                      />
                    </div>
                  </div>
                </div>
              </FaPageMain>
              <!-- 任务审批 -->
              <FaPageMain
                v-if="['待审批', '待分发', '进行中', '已完成', '已退回'].includes(order.status)"
                title=""
                :collaspe="!expandedMap[order.orderID]"
                height="auto"
                class="w-full"
              >
                <template #title>
                  <div class="w-full flex items-center justify-between pr-4">
                    <div class="flex items-center">
                      <span class="text-lg text-blue-900 font-bold">任务审批</span>
                      <!-- 状态点 -->
                      <span
                        v-if="order.status === '待审批'"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;"
                        title="进行中"
                      />
                      <span
                        v-else-if="['待分发', '进行中', '已完成'].includes(order.status)"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;"
                        title="已完成"
                      />
                      <span
                        v-else-if="order.status === '已退回'"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #ef4444;border-radius: 50%;"
                        title="已退回"
                      />
                    </div>
                    <!-- 负责人和完成时间（审批人ID和审批时间） -->
                    <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                      <span>负责人：{{ order.approverID }}</span>
                      <span>完成时间：{{ order.approveTime }}</span>
                    </div>
                  </div>
                </template>
                <div
                  class="flex items-center justify-between rounded px-6 py-4"
                  :class="order.status === '待审批' ? 'bg-gray-50' : 'bg-gray-100 opacity-70'"
                >
                  <div class="flex items-center gap-3">
                    <span class="ml-4 text-gray-700 font-semibold">参考优先级：</span>
                    <el-input
                      :model-value="order.referencePriority"
                      size="small"
                      style="width: 120px;"
                      disabled
                    />
                    <!-- 新增：下一流程负责人 -->
                    <span class="ml-4 text-gray-700 font-semibold">下一流程负责人：</span>
                    <el-input
                      :model-value="order.distributorID"
                      size="small"
                      style="width: 120px;"
                      disabled
                    />
                    <!-- 新增：已退回时显示拒绝原因 -->
                    <div v-if="order.status === '已退回'" class="flex items-center gap-3">
                      <span class="ml-4 text-red-600 font-semibold">拒绝原因：</span>
                      <el-input
                        :model-value="order.rejectReason"
                        size="small"
                        style="width: 220px;"
                        disabled
                      />
                    </div>
                  </div>
                </div>
              </FaPageMain>
              <!-- 任务发起（始终显示在最下方） -->
              <FaPageMain
                title=""
                :collaspe="!expandedMap[order.orderID]"
                height="auto"
                class="w-full"
              >
                <template #title>
                  <div class="w-full flex items-center justify-between pr-4">
                    <div class="flex items-center">
                      <span class="text-lg text-blue-900 font-bold">任务发起</span>
                      <span
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;"
                        title="已完成"
                      />
                    </div>
                    <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                      <span>负责人：{{ order.promoterID }}</span>
                      <span>完成时间：{{ order.startTime }}</span>
                    </div>
                  </div>
                </template>
                <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded bg-gray-50 px-6 py-4">
                  <!-- 基础信息 -->
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">工单ID：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold" :value="order.orderID" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">工单类型：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold" :value="order.type" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">发起人ID：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold" :value="order.promoterID" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">发起时间：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold" :value="order.startTime" readonly>
                  </div>

                  <!-- 问题复现类 -->
                  <template v-if="order.type === '问题复现'">
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">模型ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.modelID" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">模型版本ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.modelVersionID" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">对应协调单：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.coordinationID || 'NA'" readonly>
                    </div>
                    <div class="col-span-2 flex items-start gap-2">
                      <span class="w-32 text-black font-semibold">复现内容：</span>
                      <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.description" rows="2" readonly />
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">复现参考文件：</span>
                      <template v-if="order.files && order.files.length">
                        <div class="flex flex-wrap gap-2">
                          <span
                            v-for="(file, idx) in order.files"
                            :key="idx"
                            class="flex cursor-pointer items-center text-blue-600 underline hover:text-blue-800"
                            @click="downloadFile(file.fileUrl, file.fileName)"
                          >
                            <i class="i-mdi-download mr-1" />
                            {{ file.fileName }}
                          </span>
                        </div>
                      </template>
                      <span v-else class="text-gray-400">无</span>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">审批人ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.approverID || 'NA'" readonly>
                    </div>
                  </template>

                  <!-- 版本迭代类 -->
                  <template v-else-if="order.type === '版本迭代'">
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">模型ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.modelID" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">模型版本ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.modelVersionID" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">对应协调单：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.coordinationID || 'NA'" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">更新内容：</span>
                      <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.updateNotes" rows="2" readonly />
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">封装要求：</span>
                      <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.packageRequirement" rows="2" readonly />
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">接口是否变化：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.apiChanged" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">审批人ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.approverID || 'NA'" readonly>
                    </div>
                  </template>

                  <!-- 交付发送类 -->
                  <template v-else-if="order.type === '交付发送'">
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">模型ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.modelID" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">模型版本ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.modelVersionID" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">目标客户：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.targetCustomer" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">CAE平台验证：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.isCAEChecked" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">包含敏感信息：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.hasSensitiveInfo" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">审批人ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.approverID || 'NA'" readonly>
                    </div>
                  </template>

                  <!-- 版本迭代+交付发送类 -->
                  <template v-else-if="order.type === '版本迭代+交付发送'">
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">模型ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.modelID" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">模型版本ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.modelVersionID" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">对应协调单：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.coordinationID || 'NA'" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">更新内容：</span>
                      <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.updateNotes" rows="2" readonly />
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">封装要求：</span>
                      <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.packageRequirement" rows="2" readonly />
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">接口是否变化：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.apiChanged" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">目标客户：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.targetCustomer" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">CAE平台验证：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.isCAEChecked" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">包含敏感信息：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.hasSensitiveInfo" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">审批人ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.approverID || 'NA'" readonly>
                    </div>
                  </template>

                  <!-- 功能开发类 -->
                  <template v-else-if="order.type === '功能开发'">
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">模型ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.modelID" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">模型版本ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.modelVersionID" readonly>
                    </div>
                    <div class="flex items-start gap-2">
                      <span class="w-32 text-black font-semibold">功能描述：</span>
                      <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.featureDesc" rows="2" readonly />
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">审批人ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.approverID || 'NA'" readonly>
                    </div>
                  </template>

                  <!-- 其他类 -->
                  <template v-else-if="order.type === '其他'">
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">模型ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.modelID" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">模型版本ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.modelVersionID" readonly>
                    </div>
                    <div class="flex items-start gap-2">
                      <span class="w-32 text-black font-semibold">内容描述：</span>
                      <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.contentDesc" rows="2" readonly />
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">审批人ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.approverID || 'NA'" readonly>
                    </div>
                  </template>
                </div>
              </FaPageMain>
            </div>
            <div class="mt-6" />
          </FaPageMain>
        </div>
        <div class="mt-6 flex justify-center">
          <el-pagination
            background
            layout="prev, pager, next"
            :page-size="pageSize"
            :current-page="currentPage"
            :total="total"
            @current-change="handlePageChange"
          />
        </div>
        <!-- 详细信息弹窗 -->
        <el-dialog
          v-model="dialogVisible"
          title=""
          width="900px"
          :close-on-click-modal="false"
          @close="closeDialog"
        >
          <template #title>
            <span class="text-lg font-bold">工单详细信息</span>
          </template>
          <div v-if="currentOrder">
            <div class="grid grid-cols-2 gap-x-8 gap-y-4">
              <!-- 基础信息 -->
              <div class="flex items-center gap-2">
                <span class="w-32 text-black font-semibold">工单ID：</span>
                <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold" :value="currentOrder.orderID" readonly>
              </div>
              <div class="flex items-center gap-2">
                <span class="w-32 text-black font-semibold">工单类型：</span>
                <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold" :value="currentOrder.type" readonly>
              </div>
              <div class="flex items-center gap-2">
                <span class="w-32 text-black font-semibold">发起人ID：</span>
                <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold" :value="currentOrder.promoterID" readonly>
              </div>
              <div class="flex items-center gap-2">
                <span class="w-32 text-black font-semibold">发起时间：</span>
                <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold" :value="currentOrder.startTime" readonly>
              </div>

              <!-- 问题复现类 -->
              <template v-if="currentOrder.type === '问题复现'">
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">模型ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.modelID" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">模型版本ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.modelVersionID" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">对应协调单：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.coordinationID || 'NA'" readonly>
                </div>
                <div class="col-span-2 flex items-start gap-2">
                  <span class="w-32 text-black font-semibold">复现内容：</span>
                  <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.description" rows="2" readonly />
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">审批人ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.approverID || 'NA'" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">复现参考文件：</span>
                  <span v-if="currentOrder.files?.length" class="cursor-pointer text-blue-600 underline">已上传</span>
                  <span v-else class="text-gray-400">无</span>
                </div>
              </template>

              <!-- 版本迭代类 -->
              <template v-else-if="currentOrder.type === '版本迭代'">
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">模型ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.modelID" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">模型版本ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.modelVersionID" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">对应协调单：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.coordinationID || 'NA'" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">更新内容：</span>
                  <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.updateNotes" rows="2" readonly />
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">封装要求：</span>
                  <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.packageRequirement" rows="2" readonly />
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">接口是否变化：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.apiChanged" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">审批人ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.approverID || 'NA'" readonly>
                </div>
              </template>

              <!-- 交付发送类 -->
              <template v-else-if="currentOrder.type === '交付发送'">
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">模型ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.modelID" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">模型版本ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.modelVersionID" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">目标客户：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.targetCustomer" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">CAE平台验证：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.isCAEChecked" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">包含敏感信息：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.hasSensitiveInfo" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">审批人ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.approverID || 'NA'" readonly>
                </div>
              </template>

              <!-- 版本迭代+交付发送类 -->
              <template v-else-if="currentOrder.type === '版本迭代+交付发送'">
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">模型ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.modelID" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">模型版本ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.modelVersionID" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">对应协调单：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.coordinationID || 'NA'" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">更新内容：</span>
                  <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.updateNotes" rows="2" readonly />
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">封装要求：</span>
                  <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.packageRequirement" rows="2" readonly />
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">接口是否变化：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.apiChanged" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">目标客户：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.targetCustomer" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">CAE平台验证：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.isCAEChecked" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">包含敏感信息：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.hasSensitiveInfo" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">审批人ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.approverID || 'NA'" readonly>
                </div>
              </template>

              <!-- 功能开发类 -->
              <template v-else-if="currentOrder.type === '功能开发'">
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">模型ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.modelID" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">模型版本ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.modelVersionID" readonly>
                </div>
                <div class="flex items-start gap-2">
                  <span class="w-32 text-black font-semibold">功能描述：</span>
                  <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.featureDesc" rows="2" readonly />
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">审批人ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.approverID || 'NA'" readonly>
                </div>
              </template>

              <!-- 其他类 -->
              <template v-else-if="currentOrder.type === '其他'">
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">模型ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.modelID" readonly>
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">模型版本ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.modelVersionID" readonly>
                </div>
                <div class="flex items-start gap-2">
                  <span class="w-32 text-black font-semibold">内容描述：</span>
                  <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.contentDesc" rows="2" readonly />
                </div>
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">审批人ID：</span>
                  <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.approverID || 'NA'" readonly>
                </div>
              </template>

              <hr class="col-span-2 my-4 border-t border-gray-300">

              <!-- 通用流程信息 -->
              <div class="col-span-1 w-full flex items-center gap-2">
                <span class="w-32 text-black font-semibold">审批人：</span>
                <input
                  class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                  :class="['待分发', '进行中', '已完成', '已退回'].includes(currentOrder.status) && currentOrder.approverID ? 'text-blue-700' : 'text-gray-400'"
                  :value="['待分发', '进行中', '已完成', '已退回'].includes(currentOrder.status) ? (currentOrder.approverID || '无') : '未到此环节'"
                  readonly
                >
              </div>
              <div class="col-span-1 w-full flex items-center gap-2">
                <span class="w-32 text-black font-semibold">审批时间：</span>
                <input
                  class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                  :class="['待分发', '进行中', '已完成', '已退回'].includes(currentOrder.status) && currentOrder.approveTime ? 'text-blue-700' : 'text-gray-400'"
                  :value="['待分发', '进行中', '已完成', '已退回'].includes(currentOrder.status) ? (currentOrder.approveTime || '无') : '未到此环节'"
                  readonly
                >
              </div>
              <div class="col-span-1 w-full flex items-center gap-2">
                <span class="w-32 text-black font-semibold">分发人：</span>
                <input
                  class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                  :class="['进行中', '已完成', '已退回'].includes(currentOrder.status) && currentOrder.distributorID ? 'text-blue-700' : 'text-gray-400'"
                  :value="['进行中', '已完成', '已退回'].includes(currentOrder.status) ? (currentOrder.distributorID || '无') : '未到此环节'"
                  readonly
                >
              </div>
              <div class="col-span-1 w-full flex items-center gap-2">
                <span class="w-32 text-black font-semibold">分发时间：</span>
                <input
                  class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                  :class="['进行中', '已完成', '已退回'].includes(currentOrder.status) && currentOrder.distributeTime ? 'text-blue-700' : 'text-gray-400'"
                  :value="['进行中', '已完成', '已退回'].includes(currentOrder.status) ? (currentOrder.distributeTime || '无') : '未到此环节'"
                  readonly
                >
              </div>
              <div class="col-span-1 w-full flex items-center gap-2">
                <span class="w-32 text-black font-semibold">执行人ID：</span>
                <input
                  class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                  :class="['进行中', '已完成', '已退回'].includes(currentOrder.status) && currentOrder.executorID ? 'text-blue-700' : 'text-gray-400'"
                  :value="['进行中', '已完成', '已退回'].includes(currentOrder.status) ? (currentOrder.executorID || '无') : '未到此环节'"
                  readonly
                >
              </div>
              <div class="col-span-1 w-full flex items-center gap-2">
                <span class="w-32 text-black font-semibold">工单完成类型：</span>
                <input
                  class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                  :class="currentOrder.status === '已完成' && currentOrder.type ? 'text-blue-700' : 'text-gray-400'"
                  :value="currentOrder.status === '已完成' ? (currentOrder.type || '无') : '未到此环节'"
                  readonly
                >
              </div>

              <!-- 完成类型细分：仅已完成时显示 -->
              <template v-if="currentOrder.status === '已完成'">
                <template v-if="currentOrder.type === '问题复现'">
                  <!-- 问题复现类工单 -->
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">复现现象：</span>
                    <textarea
                      class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="currentOrder.finishPhenomenon" rows="2" readonly
                    />
                  </div>
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">备注：</span>
                    <textarea
                      class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="currentOrder.finishRemark" rows="2" readonly
                    />
                  </div>
                </template>

                <!-- 版本迭代类工单 -->
                <template v-else-if="currentOrder.type === '版本迭代'">
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">升级后模型版本：</span>
                    <input
                      class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="currentOrder.finishModelVersion" readonly
                    >
                  </div>
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">备注：</span>
                    <textarea
                      class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="currentOrder.finishRemark" rows="2" readonly
                    />
                  </div>
                </template>

                <!-- 交付发送类工单 -->
                <template v-else-if="currentOrder.type === '交付发送'">
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">是否加密：</span>
                    <input
                      class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="currentOrder.isEncrypted"
                      readonly
                    >
                  </div>
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">外壳号：</span>
                    <input
                      class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="currentOrder.finishShellNo"
                      readonly
                    >
                  </div>
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">授权ID：</span>
                    <input
                      class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="currentOrder.finishAuthId"
                      readonly
                    >
                  </div>
                  <div class="col-span-2 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">备注：</span>
                    <textarea
                      class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="currentOrder.finishRemark"
                      rows="2"
                      readonly
                    />
                  </div>
                </template>

                <!-- 版本迭代+交付发送类工单 -->
                <template v-if="currentOrder.type === '版本迭代+交付发送'">
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">升级后模型版本：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.finishModelVersion" readonly>
                  </div>
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">是否加密：</span>
                    <input
                      class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="currentOrder.isEncrypted"
                      readonly
                    >
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">授权ID：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.finishAuthId" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">外壳号：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.finishShellNo" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">备注：</span>
                    <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="currentOrder.finishRemark" rows="2" readonly />
                  </div>
                </template>

                <!-- 功能开发类工单 -->
                <template v-else-if="currentOrder.type === '功能开发'">
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">升级后模型版本：</span>
                    <input
                      class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="currentOrder.finishModelVersionId" readonly
                    >
                  </div>
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">完成功能描述：</span>
                    <textarea
                      class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="currentOrder.finishFeatureDesc" rows="2" readonly
                    />
                  </div>
                </template>

                <!-- 其他类工单 -->
                <template v-else-if="currentOrder.type === '其他'">
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">备注：</span>
                    <textarea
                      class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="currentOrder.finishRemark" rows="2" readonly
                    />
                  </div>
                </template>
              </template>

              <div class="col-span-1 w-full flex items-center gap-2">
                <span class="w-32 text-black font-semibold">完成时间：</span>
                <input
                  class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                  :class="currentOrder.status === '已完成' && currentOrder.finishTime ? 'text-blue-700' : 'text-gray-400'"
                  :value="currentOrder.status === '已完成' ? (currentOrder.finishTime || '无') : '未到此环节'"
                  readonly
                >
              </div>
            </div>
          </div>
        </el-dialog>
      </template>
    </el-skeleton>
  </div>
</template>
