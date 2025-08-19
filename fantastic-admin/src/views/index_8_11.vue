<script setup lang="ts">
import { ElMessage } from 'element-plus'
import { onMounted, ref } from 'vue'
import deliveryApi from '@/api/modules/delivery' // 新增：导入客户管理API
import orderApi from '@/api/modules/order'
import { useUserStore } from '@/store/modules/user'

// -----------------变量定义-----------------
const loading = ref(false) // loading变量定义
const userOrders = ref<OrderItem[]>([]) // 后端返回的待办工单列表
const totalPendingCount = ref(0) // 总待办工单数量
const recentOrders = ref<OrderItem[]>([]) // 用户近期发送的工单列表
const recentOrdersLoading = ref(false) // 近期工单加载状态

// 新增：授权模块相关变量
const clientList = ref<Client[]>([]) // 客户列表
const clientLoading = ref(false) // 客户数据加载状态

const userStore = useUserStore()

// ----------------数据结构定义-----------------
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
}

// 定义客户信息数据结构
interface Client {
  clientName: string // 客户名称
  dongleCount: number // 加密狗数量
  modelCount: number // 发送模型数量
  modelVersionCount: number // 发送模型总版本数量
  clientinfo?: string // 客户信息备注

  // 授权ID数量统计
  licenseStats: {
    validCount: number // 有效授权数量
    expiringCount: number // 临期授权数量（7天内过期）
    expiredCount: number // 过期授权数量
  }
}

// 获取用户待办工单信息
async function apiFetchUserOrders() {
  try {
    // 构造请求参数，获取当前用户的待办工单
    const params = {
      userID: userStore.account || '', // 当前用户ID
      role: userStore.role || '', // 当前用户角色
    }

    // 调用后端接口，获取数据
    const res = await orderApi.fetchUserOrderList(params)

    // 处理返回的数据
    const orders = res.data.list || []

    // 过滤出待办状态的工单
    const pendingOrders = orders.filter((order: any) =>
      order.status && ['草稿', '待审批', '待分发', '进行中'].includes(order.status),
    )

    // 设置总待办工单数量
    totalPendingCount.value = pendingOrders.length

    // 只取前三个待办工单
    const limitedOrders = pendingOrders.slice(0, 3)

    // 数据映射处理 - 将后端字段映射到前端期望的字段
    return limitedOrders.map((order: any) => {
      const mappedOrder: OrderItem = {
        // 基础字段映射
        orderID: order.orderID || '', // 工单ID
        type: order.type || '其他', // 工单类型
        status: order.status || '草稿', // 工单状态
        referencePriority: order.referencePriority || '', // 参考优先级
        taskPriority: order.taskPriority || '', // 任务优先级
        modelID: order.modelID || '', // 模型ID
        modelVersionID: order.modelVersionID || '', // 模型版本ID
        promoterID: order.promoterID || '', // 创建人ID
        startTime: order.startTime || '', // 创建时间
      }

      return mappedOrder
    })
  }
  catch (error) {
    console.error('获取用户待办工单失败:', error)
    // 发生错误时返回空数组
    return []
  }
}

// 获取用户近期发送的工单
async function apiFetchRecentOrders() {
  try {
    // 构造请求参数，获取当前用户发送的工单
    const params = {
      userID: userStore.account || '', // 当前用户作为发起人
    }

    // 调用后端接口，获取数据
    const res = await orderApi.fetchUserRecentOrderList(params)

    // 处理返回的数据
    const orders = res.data.list || []

    // 按时间排序，取最近的3条
    const sortedOrders = orders.sort((a: any, b: any) => {
      return new Date(b.startTime).getTime() - new Date(a.startTime).getTime()
    }).slice(0, 3)

    // 数据映射处理
    return sortedOrders.map((order: any) => {
      const mappedOrder: OrderItem = {
        orderID: order.orderID || '',
        type: order.type || '其他',
        status: order.status || '草稿',
        referencePriority: order.referencePriority || '',
        taskPriority: order.taskPriority || '',
        modelID: order.modelID || '',
        modelVersionID: order.modelVersionID || '',
        promoterID: order.promoterID || '',
        startTime: order.startTime || '',
      }

      return mappedOrder
    })
  }
  catch (error) {
    console.error('获取用户近期发送工单失败:', error)
    ElMessage.error('获取用户近期发送工单失败')
    return []
  }
}

// 获取客户列表数据
async function fetchClients() {
  clientLoading.value = true
  try {
    // 调用客户管理API接口
    const res = await deliveryApi.getClientList()

    if (res?.data) {
      // 数据映射处理，确保与前端接口一致
      const mappedData: Client[] = (res.data.list || []).map((client: any) => ({
        clientName: client.clientName || '', // 客户名称
        dongleCount: client.dongleCount || 0, // 加密狗数量
        modelCount: client.modelCount || 0, // 发送模型数量
        modelVersionCount: client.modelVersionCount || 0, // 发送模型总版本数量
        clientinfo: client.clientinfo || '', // 客户信息备注
        licenseStats: {
          validCount: client.licenseStats?.validCount || 0, // 有效授权数量
          expiringCount: client.licenseStats?.expiringCount || 0, // 临期授权数量
          expiredCount: client.licenseStats?.expiredCount || 0, // 过期授权数量
        },
      }))

      clientList.value = mappedData
    }
    else {
      throw new Error('响应数据为空')
    }
  }
  catch (error) {
    console.error('获取客户列表失败:', error)
    ElMessage.error('获取客户列表失败')
  }
  finally {
    clientLoading.value = false
  }
}

// 调用接口，数据没有返回时显示loading
async function fetchUserOrders() {
  loading.value = true
  try {
    userOrders.value = await apiFetchUserOrders()
  }
  catch (error) {
    console.error('获取待办工单失败:', error)
    ElMessage.error('获取待办工单失败')
  }
  finally {
    loading.value = false
  }
}

// 获取近期发送的工单
async function fetchRecentOrders() {
  recentOrdersLoading.value = true
  try {
    recentOrders.value = await apiFetchRecentOrders()
  }
  catch (error) {
    console.error('获取近期发送工单失败:', error)
    ElMessage.error('获取近期发送工单失败')
  }
  finally {
    recentOrdersLoading.value = false
  }
}

// 计算授权总数
function getTotalLicenseCount(client: Client): number {
  return client.licenseStats.validCount + client.licenseStats.expiringCount + client.licenseStats.expiredCount
}

// 计算有临期授权的客户信息
const expiringClientsInfo = computed(() => {
  const expiringClients = clientList.value.filter(client => client.licenseStats.expiringCount > 0)
  return expiringClients.map(client => ({
    name: client.clientName,
    count: client.licenseStats.expiringCount,
  }))
})

// 判断客户是否有临期授权
function hasExpiringLicense(client: Client): boolean {
  return client.licenseStats.expiringCount > 0
}

// 页面加载时获取工单
onMounted(() => {
  fetchUserOrders()
  fetchRecentOrders()
  fetchClients() // 新增：获取客户数据
})
</script>

<template>
  <div>
    <!-- 顶部标题栏 -->
    <div class="mb-6 border border-gray-200 rounded-lg bg-white p-6 shadow-sm">
      <div class="flex items-center justify-between">
        <div class="flex items-center">
          <h1 class="text-2xl text-gray-800 font-bold">
            主页
          </h1>
        </div>
      </div>

      <!-- 文字提示行 -->
      <div class="mt-3 text-sm text-gray-600">
        本页面显示您的待办工单信息，您可以查看和管理相关工单。
      </div>
    </div>

    <!-- 待办工单信息内容块 -->
    <FaPageMain class="mb-6">
      <template #title>
        <div class="flex items-center gap-2">
          <i class="i-mdi-clipboard-list-outline text-2xl text-blue-500" />
          <span class="text-xl font-bold">我的待办工单</span>
          <!-- 显示信息提示 - 修改为显示总数量 -->
          <span class="flex items-center gap-1 text-base text-gray-500 font-medium">
            <i class="i-mdi-information-outline text-blue-500" />
            (当前显示前{{ Math.min(3, totalPendingCount) }}条待办工单，共{{ totalPendingCount }}条)
          </span>
        </div>
      </template>

      <el-skeleton :loading="loading" animated>
        <template #template>
          <div class="space-y-4">
            <div v-for="i in 3" :key="i" class="border rounded-lg p-4">
              <el-skeleton-item variant="text" style="width: 60%;" />
              <div class="mt-2 space-y-2">
                <el-skeleton-item variant="text" style="width: 80%;" />
                <el-skeleton-item variant="text" style="width: 70%;" />
              </div>
            </div>
          </div>
        </template>

        <!-- 添加 #default 模板 -->
        <template #default>
          <div v-if="userOrders.length === 0" class="py-8 text-center text-gray-500">
            <i class="i-mdi-clipboard-outline mb-2 text-4xl" />
            <p>暂无待办工单</p>
          </div>

          <div v-else class="space-y-4">
            <!-- 每一条待办工单 -->
            <FaPageMain
              v-for="order in userOrders"
              :key="order.orderID"
              class="border border-gray-200"
            >
              <div class="w-full flex items-center justify-between">
                <div>
                  <span class="block flex items-center text-lg text-black font-extrabold tracking-wide">
                    工单#{{ order.orderID }}
                    <span
                      class="ml-4 align-middle text-base font-bold"
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
                  </span>
                  <div class="mt-2 flex flex-wrap items-center gap-6 text-sm">
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
                      <span class="text-black font-bold">{{ order.referencePriority || '无' }}</span>
                    </span>
                    <span>
                      <i class="i-mdi-alert mr-1 text-blue-400" />
                      <span class="text-gray-600">任务优先级：</span>
                      <span class="text-black font-bold">{{ order.taskPriority || '无' }}</span>
                    </span>
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
                  </div>
                </div>
              </div>
            </FaPageMain>
          </div>
        </template>
      </el-skeleton>
    </FaPageMain>

    <!-- 近期发送工单信息内容块 -->
    <FaPageMain>
      <template #title>
        <div class="flex items-center gap-2">
          <i class="i-mdi-send-outline text-2xl text-green-500" />
          <span class="text-xl font-bold">我近期发送的工单</span>
          <!-- 显示信息提示 -->
          <span class="flex items-center gap-1 text-base text-gray-500 font-medium">
            <i class="i-mdi-information-outline text-green-500" />
            (显示最近{{ recentOrders.length }}条发送的工单)
          </span>
        </div>
      </template>

      <el-skeleton :loading="recentOrdersLoading" animated>
        <template #template>
          <div class="space-y-4">
            <div v-for="i in 5" :key="i" class="border rounded-lg p-4">
              <el-skeleton-item variant="text" style="width: 60%;" />
              <div class="mt-2 space-y-2">
                <el-skeleton-item variant="text" style="width: 80%;" />
                <el-skeleton-item variant="text" style="width: 70%;" />
              </div>
            </div>
          </div>
        </template>

        <template #default>
          <div v-if="recentOrders.length === 0" class="py-8 text-center text-gray-500">
            <i class="i-mdi-send-outline mb-2 text-4xl" />
            <p>暂无近期发送的工单</p>
          </div>

          <div v-else class="space-y-4">
            <!-- 每一条近期发送的工单 -->
            <FaPageMain
              v-for="order in recentOrders"
              :key="`recent-${order.orderID}`"
              class="border border-gray-200"
            >
              <div class="w-full flex items-center justify-between">
                <div>
                  <span class="block flex items-center text-lg text-black font-extrabold tracking-wide">
                    工单#{{ order.orderID }}
                    <span
                      class="ml-4 align-middle text-base font-bold"
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
                  </span>
                  <div class="mt-2 flex flex-wrap items-center gap-6 text-sm">
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
                      <span class="text-black font-bold">{{ order.referencePriority || '无' }}</span>
                    </span>
                    <span>
                      <i class="i-mdi-alert mr-1 text-blue-400" />
                      <span class="text-gray-600">任务优先级：</span>
                      <span class="text-black font-bold">{{ order.taskPriority || '无' }}</span>
                    </span>
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
                  </div>
                </div>
              </div>
            </FaPageMain>
          </div>
        </template>
      </el-skeleton>
    </FaPageMain>

    <!-- 新增：客户授权统计模块 -->
    <FaAuth value="">
      <FaPageMain>
        <template #title>
          <div class="flex items-center gap-2">
            <i class="i-mdi-shield-key-outline text-2xl text-orange-500" />
            <span class="text-xl font-bold">客户授权统计</span>
            <!-- 显示信息提示 -->
            <span class="flex items-center gap-1 text-base text-gray-500 font-medium">
              <i class="i-mdi-information-outline text-orange-500" />
              (显示所有客户的授权信息概览)
            </span>
          </div>
        </template>

        <el-skeleton :loading="clientLoading" animated>
          <template #template>
            <div class="space-y-4">
              <div v-for="i in 3" :key="i" class="border rounded-lg p-4">
                <el-skeleton-item variant="text" style="width: 40%;" />
                <div class="mt-4 flex gap-4">
                  <el-skeleton-item variant="text" style="width: 20%;" />
                  <el-skeleton-item variant="text" style="width: 20%;" />
                  <el-skeleton-item variant="text" style="width: 20%;" />
                  <el-skeleton-item variant="text" style="width: 20%;" />
                </div>
              </div>
            </div>
          </template>

          <template #default>
            <div v-if="clientList.length === 0" class="py-8 text-center text-gray-500">
              <i class="i-mdi-shield-outline mb-2 text-4xl" />
              <p>暂无客户授权信息</p>
            </div>

            <div v-else class="space-y-4">
              <!-- 新增：临期授权提醒信息 -->
              <div v-if="expiringClientsInfo.length > 0" class="mb-4 border border-orange-200 rounded-lg bg-orange-50 p-4">
                <div class="flex items-start">
                  <i class="i-mdi-alert-circle-outline mr-2 mt-0.5 text-lg text-orange-500" />
                  <div>
                    <div class="text-sm text-orange-800 font-medium">
                      授权到期提醒
                    </div>
                    <div class="mt-1 text-sm text-orange-700">
                      <template v-for="(client, index) in expiringClientsInfo" :key="client.name">
                        客户{{ client.name }}有{{ client.count }}个授权快过期<span v-if="index < expiringClientsInfo.length - 1">，</span>
                      </template>
                    </div>
                  </div>
                </div>
              </div>
              <!-- 每个客户占据一行 -->
              <div
                v-for="client in clientList"
                :key="client.clientName"
                class="border border-gray-200 rounded-lg bg-white p-4 transition-shadow hover:shadow-md"
              >
                <!-- 客户名称行 - 修改标记点颜色逻辑 -->
                <div class="mb-4 flex items-center">
                  <div
                    class="mr-3 h-3 w-3 rounded-full"
                    :class="{
                      'bg-orange-500': hasExpiringLicense(client),
                      'bg-green-500': !hasExpiringLicense(client),
                    }"
                  />
                  <span class="text-lg text-black font-bold">{{ client.clientName }}</span>
                  <span v-if="client.clientinfo" class="ml-3 text-sm text-gray-500">
                    ({{ client.clientinfo }})
                  </span>
                  <!-- 新增：临期授权标识 -->
                  <span v-if="hasExpiringLicense(client)" class="ml-2 rounded-full bg-orange-100 px-2 py-1 text-xs text-orange-700 font-medium">
                    {{ client.licenseStats.expiringCount }}个临期
                  </span>
                </div>

                <!-- 统计信息行 -->
                <div class="grid grid-cols-5 gap-6 text-center">
                  <!-- 加密锁数量 -->
                  <div class="flex flex-col items-center">
                    <div class="text-2xl text-blue-600 font-bold">
                      {{ client.dongleCount }}
                    </div>
                    <div class="mt-1 text-sm text-gray-600">
                      加密锁数量
                    </div>
                  </div>

                  <!-- 有效授权 -->
                  <div class="flex flex-col items-center">
                    <div class="text-2xl text-green-500 font-bold">
                      {{ client.licenseStats.validCount }}
                    </div>
                    <div class="mt-1 text-sm text-gray-600">
                      有效授权
                    </div>
                  </div>

                  <!-- 临期授权 -->
                  <div class="flex flex-col items-center">
                    <div class="text-2xl text-yellow-500 font-bold">
                      {{ client.licenseStats.expiringCount }}
                    </div>
                    <div class="mt-1 text-sm text-gray-600">
                      临期授权
                    </div>
                  </div>

                  <!-- 过期授权 -->
                  <div class="flex flex-col items-center">
                    <div class="text-2xl text-gray-500 font-bold">
                      {{ client.licenseStats.expiredCount }}
                    </div>
                    <div class="mt-1 text-sm text-gray-600">
                      过期授权
                    </div>
                  </div>

                  <!-- 总授权数 -->
                  <div class="flex flex-col items-center">
                    <div class="text-2xl text-black font-bold">
                      {{ getTotalLicenseCount(client) }}
                    </div>
                    <div class="mt-1 text-sm text-gray-600">
                      总授权数
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </template>
        </el-skeleton>
      </FaPageMain>

      <!-- 无权限时的提示信息 -->
      <template #no-auth>
        <FaPageMain>
          <template #title>
            <div class="flex items-center gap-2">
              <i class="i-mdi-lock-outline text-2xl text-gray-400" />
              <span class="text-xl text-gray-400 font-bold">客户授权统计</span>
            </div>
          </template>

          <div class="py-8 text-center text-gray-400">
            <i class="i-mdi-lock-outline mb-2 text-4xl" />
            <p>您没有查看客户授权信息的权限</p>
            <p class="mt-2 text-sm">
              请联系管理员获取相应权限
            </p>
          </div>
        </FaPageMain>
      </template>
    </FaAuth>
  </div>
</template>

<style scoped>
.el-badge {
  --el-badge-bg-color: #409eff;
}
</style>
