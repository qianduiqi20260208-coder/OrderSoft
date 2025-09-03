<script setup lang="ts">
import * as echarts from 'echarts'
import { ElMessage } from 'element-plus'
import { computed, onMounted, onUnmounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import deliveryApi from '@/api/modules/delivery'
import orderApi from '@/api/modules/order'
import { useUserStore } from '@/store/modules/user'

// -----------------变量定义-----------------
const router = useRouter()
const loading = ref(false)
const userOrders = ref<OrderItem[]>([])
const myRelatedOrders = ref<OrderItem[]>([]) // 与我相关的工单
const myRelatedOrdersLoading = ref(false) // 与我相关的工单加载状态
const totalPendingCount = ref(0)
const recentOrders = ref<OrderItem[]>([])
const recentOrdersLoading = ref(false)

// 授权模块相关变量
const clientList = ref<Client[]>([])
const clientLoading = ref(false)

// 新增：图表统计模块相关变量
const chartLoading = ref(false)
const chartContainer = ref<HTMLElement>() // 图表容器引用
const chartInstance = ref<echarts.ECharts>() // 图表实例
const selectedTimePeriod = ref<'7days' | '1month' | '6months'>('7days') // 选择的时间范围
const selectedOrderType = ref<'版本迭代' | '交付发送'>('版本迭代') // 选择的工单类型
const chartStatistics = ref<ChartStatistics>({
  daily: [],
  weekly: [],
  monthly: [],
})
// 客户选择状态
const selectedClients = ref<string[]>([])
const availableClients = computed(() => {
  return clientList.value.map(client => client.clientName)
})

const userStore = useUserStore()

// ----------------数据结构定义-----------------
// 定义工单所包括的内容
interface OrderItem {
  type: string // 工单类型
  status: string // 工单状态
  referencePriority?: string // 参考优先级
  taskPriority?: string // 任务优先级
  orderID: string // 工单ID
  modelID: string // 模型名
  modelVersionID: string // 模型版本号
  promoterID: string // 发起人ID
  startTime: string // 发起时间
  statusTodo?: string // 流程状态
}

// 定义客户信息数据结构
interface Client {
  clientName: string // 客户名称
  dongleCount: number // 加密狗数量
  modelCount: number // 发送模型数量
  modelVersionCount: number // 发送模型版本数量
  clientinfo?: string // 客户信息
  licenseStats: {
    validCount: number // 有效授权数量
    expiringCount: number // 即将过期授权数量
    expiredCount: number // 已过期授权数量
  }
}

// 定义按天统计的数据结构（近7天）
interface DailyChartData {
  date: string // 日期 YYYY-MM-DD
  clientName: string // 客户名称
  versionIterationCount: number // 版本迭代工单数量
  deliveryCount: number // 交付发送工单数量
}

// 定义按周统计的数据结构（近1月）
interface WeeklyChartData {
  weekLabel: string // 周标签，如 "第1周"、"第2周"、"第3周"、"第4周(本周)"
  clientName: string // 客户名称
  versionIterationCount: number // 版本迭代工单数量
  deliveryCount: number // 交付发送工单数量
}

// 定义按月统计的数据结构（近半年）
interface MonthlyChartData {
  monthLabel: string // 月份标签，如 "2024-08"
  clientName: string // 客户名称
  versionIterationCount: number // 版本迭代工单数量
  deliveryCount: number // 交付发送工单数量
}

// 定义统一的图表数据结构
interface ChartStatistics {
  daily: DailyChartData[] // 近7天数据
  weekly: WeeklyChartData[] // 近1月数据
  monthly: MonthlyChartData[] // 近半年数据
}

// 新增：定义时间段选项
const timePeriodOptions = [
  { label: '近7天', value: '7days' as const },
  { label: '近1月', value: '1month' as const },
  { label: '近半年', value: '6months' as const },
]

// 新增：定义工单类型选项
const orderTypeOptions = [
  { label: '版本迭代', value: '版本迭代' as const },
  { label: '交付发送', value: '交付发送' as const },
]

// -----------------API函数-----------------
// 获取用户待办工单信息
async function apiFetchUserOrders() {
  try {
    const params = {
      userID: userStore.account || '',
      role: userStore.role || '',
    }

    const res = await orderApi.fetchUserOrderList(params)
    console.warn(res)
    const orders = res.data.list || []

    const pendingOrders = orders.filter((order: any) =>
      order.workOrderStatus && ['草稿', '待审批', '待分发', '进行中'].includes(order.workOrderStatus),
    )

    totalPendingCount.value = pendingOrders.length
    const limitedOrders = pendingOrders.slice(0, 3)

    return limitedOrders.map((order: any) => {
      const mappedOrder: OrderItem = {
        orderID: order.workOrderId || '',
        type: order.workOrderType === '直接封装+发送' ? '版本迭代+交付发送' : order.workOrderType || '其他',
        status: order.workOrderStatus || '草稿',
        referencePriority: order.priority || '',
        taskPriority: order.taskPriority || '',
        modelID: order.model || '',
        modelVersionID: order.modelVersion || '',
        promoterID: order.creatorName || '',
        startTime: order.createdAt || '',
        statusTodo: order.statusTodo || '',
      }
      return mappedOrder
    })
  }
  catch (error) {
    console.error('获取用户待办工单失败:', error)
    return []
  }
}

// 获取用户近期发送的工单
async function apiFetchRecentOrders() {
  try {
    const params = {
      page: 1,
      pageSize: 10, // 可根据需求调整数量
      promoterID: userStore.account || '',
    }

    const res = await orderApi.fetchOrderPage(params)
    const orders = res.data.list || []

    const sortedOrders = orders
      .sort((a: any, b: any) => new Date(b.startTime).getTime() - new Date(a.startTime).getTime())
      .slice(0, 3)

    return sortedOrders.map((order: any) => {
      const mappedOrder: OrderItem = {
        orderID: order.workOrderId || '',
        type: order.workOrderType || '其他',
        status: order.workOrderStatus || '草稿',
        referencePriority: order.priority || '',
        taskPriority: order.taskPriority || '',
        modelID: order.model || '',
        modelVersionID: order.modelVersion || '',
        promoterID: order.creatorName || '',
        startTime: order.createdAt || '',
      }
      return mappedOrder
    })
  }
  catch (error) {
    console.error('获取用户近期发送工单失败:', error)
    return []
  }
}

// 获取与我相关的工单
async function apiFetchMyRelatedOrders() {
  try {
    const params = {
      page: 1,
      pageSize: 3,
      userId: userStore.account || '',
      filterMineFlag: true,
    }

    const res = await orderApi.fetchOrderPage(params)
    const orders = res.data.list || []

    return orders.map((order: any) => {
      const mappedOrder: OrderItem = {
        orderID: order.workOrderId || '',
        type: order.workOrderType === '直接封装+发送' ? '版本迭代+交付发送' : order.workOrderType || '其他',
        status: order.workOrderStatus || '草稿',
        referencePriority: order.priority || '',
        taskPriority: order.taskPriority || '',
        modelID: order.model || '',
        modelVersionID: order.modelVersion || '',
        promoterID: order.creatorName || '',
        startTime: order.createdAt || '',
        statusTodo: order.statusTodo || '',
      }
      return mappedOrder
    })
  }
  catch (error) {
    console.error('获取与我相关的工单失败:', error)
    return []
  }
}

// 新增：获取图表统计数据
async function fetchChartData() {
  chartLoading.value = true
  try {
    if (!chartContainer.value) {
      console.error('图表容器仍未找到，跳过图表渲染')
      return
    }

    const clientNames = selectedClients.value

    // 调用新接口
    const res = await orderApi.fetchOrderStatisticsByCondition(clientNames)

    // 手动数据映射，确保数据结构正确
    if (res?.data) {
      // 映射每日数据
      const mappedDaily: DailyChartData[] = (res.data.daily || []).map((item: any) => ({
        date: item.date || '',
        clientName: item.clientName || '',
        versionIterationCount: Number(item.versionIterationCount) || 0,
        deliveryCount: Number(item.deliveryCount) || 0,
      }))

      // 映射周数据
      const mappedWeekly: WeeklyChartData[] = (res.data.weekly || []).map((item: any) => ({
        weekLabel: item.weekLabel || '',
        clientName: item.clientName || '',
        versionIterationCount: Number(item.versionIterationCount) || 0,
        deliveryCount: Number(item.deliveryCount) || 0,
      }))

      // 映射月数据
      const mappedMonthly: MonthlyChartData[] = (res.data.monthly || []).map((item: any) => ({
        monthLabel: item.monthLabel || '',
        clientName: item.clientName || '',
        versionIterationCount: Number(item.versionIterationCount) || 0,
        deliveryCount: Number(item.deliveryCount) || 0,
      }))

      chartStatistics.value = {
        daily: mappedDaily,
        weekly: mappedWeekly,
        monthly: mappedMonthly,
      }
      console.warn('图表数据加载成功:', chartStatistics.value)
    }
    else {
      throw new Error('响应数据为空')
    }
  }
  catch (error) {
    console.error('获取图表数据失败:', error)
    ElMessage.error('获取图表数据失败')
  }
  finally {
    chartLoading.value = false
  }
}

const colors = [
  '#5470c6',
  '#91cc75',
  '#fac858',
  '#ee6666',
  '#73c0de',
  '#3ba272',
  '#fc8452',
  '#9a60b4',
  '#ea7ccc',
  '#2ec7c9',
  '#b6a2de',
  '#5ab1ef',
  '#ffb980',
  '#d87a80',
  '#8d98b3',
  '#e5cf0d',
  '#97b552',
  '#95706d',
  '#dc69aa',
  '#07a2a4',
  '#9a7fd1',
  '#588dd5',
  '#f5994e',
  '#c05050',
  '#59678c',
  '#c9ab00',
  '#7eb00a',
  '#6f5553',
  '#c14089',
]

// 修改：渲染图表函数
function renderChart() {
  if (!chartContainer.value) {
    console.error('图表容器未找到')
    return
  }

  try {
    if (chartInstance.value) {
      chartInstance.value.dispose()
    }

    chartInstance.value = echarts.init(chartContainer.value)

    // 根据时间范围获取对应数据
    let currentData: any[] = []
    let xAxisData: string[] = []
    let series: any[] = []

    if (selectedTimePeriod.value === '7days') {
      currentData = chartStatistics.value.daily
      const dateGroups = [...new Set(currentData.map(item => item.date))].sort()
      xAxisData = dateGroups.map((date) => {
        const d = new Date(date)
        return `${d.getMonth() + 1}/${d.getDate()}`
      })

      if (selectedOrderType.value === '版本迭代') {
        // 只显示第一个客户的版本迭代数据（因为每个客户数据都一样）
        const firstClient = currentData.length > 0 ? currentData[0].clientName : ''
        const data = dateGroups.map((date) => {
          const item = currentData.find(d => d.date === date && d.clientName === firstClient)
          return item ? item.versionIterationCount || 0 : 0
        })
        series = [{
          name: '版本迭代',
          type: 'line',
          smooth: false,
          symbol: 'circle',
          symbolSize: 6,
          lineStyle: { width: 3 },
          itemStyle: { color: '#5470c6' },
          label: {
            show: true,
            position: 'top',
            formatter: '{c}',
            fontSize: 12,
            fontWeight: 'bold',
            color: '#5470c6',
            offset: [0, -5],
          },
          data,
        }]
      }

      else {
        // 交付发送工单按客户分组
        const clientGroups: Record<string, number[]> = {}
        selectedClients.value.forEach((client) => {
          clientGroups[client] = []
        })
        dateGroups.forEach((date) => {
          selectedClients.value.forEach((client) => {
            const item = currentData.find(d => d.date === date && d.clientName === client)
            clientGroups[client].push(item?.deliveryCount || 0)
          })
        })
        series = selectedClients.value.map((clientName, index) => ({
          name: clientName,
          type: 'line',
          smooth: false,
          symbol: 'circle',
          symbolSize: 6,
          lineStyle: { width: 3 },
          itemStyle: { color: colors[index % colors.length] },
          label: {
            show: true,
            position: 'top',
            formatter: '{c}',
            fontSize: 12,
            fontWeight: 'bold',
            color: colors[index % colors.length],
            offset: [0, -5],
          },
          data: clientGroups[clientName],
        }))
      }
    }
    else if (selectedTimePeriod.value === '1month') {
      currentData = chartStatistics.value.weekly
      const weekNumbers = [...new Set(currentData.map(item => item.weekLabel))].sort((a, b) => Number(a) - Number(b))
      xAxisData = weekNumbers.map((w, i) => i === weekNumbers.length - 1 ? `第${w}周(本周)` : `第${w}周`)

      if (selectedOrderType.value === '版本迭代') {
        const firstClient = currentData.length > 0 ? currentData[0].clientName : ''
        const data = weekNumbers.map((weekLabel) => {
          const item = currentData.find(d => d.weekLabel === weekLabel && d.clientName === firstClient)
          return item ? item.versionIterationCount || 0 : 0
        })
        series = [{
          name: '版本迭代',
          type: 'line',
          smooth: false,
          symbol: 'circle',
          symbolSize: 6,
          lineStyle: { width: 3 },
          itemStyle: { color: '#5470c6' },
          label: {
            show: true,
            position: 'top',
            formatter: '{c}',
            fontSize: 12,
            fontWeight: 'bold',
            color: '#5470c6',
            offset: [0, -5],
          },
          data,
        }]
      }
      else {
        const clientGroups: Record<string, number[]> = {}
        selectedClients.value.forEach((client) => {
          clientGroups[client] = []
        })
        weekNumbers.forEach((weekLabel) => {
          selectedClients.value.forEach((client) => {
            const item = currentData.find(d => d.weekLabel === weekLabel && d.clientName === client)
            clientGroups[client].push(item?.deliveryCount || 0)
          })
        })
        series = selectedClients.value.map((clientName, index) => ({
          name: clientName,
          type: 'line',
          smooth: false,
          symbol: 'circle',
          symbolSize: 6,
          lineStyle: { width: 3 },
          itemStyle: { color: colors[index % colors.length] },
          label: {
            show: true,
            position: 'top',
            formatter: '{c}',
            fontSize: 12,
            fontWeight: 'bold',
            color: colors[index % colors.length],
            offset: [0, -5],
          },
          data: clientGroups[clientName],
        }))
      }
    }
    else {
      currentData = chartStatistics.value.monthly
      const monthGroups = [...new Set(currentData.map(item => item.monthLabel))].sort()
      xAxisData = monthGroups

      if (selectedOrderType.value === '版本迭代') {
        const firstClient = currentData.length > 0 ? currentData[0].clientName : ''
        const data = monthGroups.map((monthLabel) => {
          const item = currentData.find(d => d.monthLabel === monthLabel && d.clientName === firstClient)
          return item ? item.versionIterationCount || 0 : 0
        })
        series = [{
          name: '版本迭代',
          type: 'line',
          smooth: false,
          symbol: 'circle',
          symbolSize: 6,
          lineStyle: { width: 3 },
          itemStyle: { color: '#5470c6' },
          label: {
            show: true,
            position: 'top',
            formatter: '{c}',
            fontSize: 12,
            fontWeight: 'bold',
            color: '#5470c6',
            offset: [0, -5],
          },
          data,
        }]
      }
      else {
        const clientGroups: Record<string, number[]> = {}
        selectedClients.value.forEach((client) => {
          clientGroups[client] = []
        })
        monthGroups.forEach((monthLabel) => {
          selectedClients.value.forEach((client) => {
            const item = currentData.find(d => d.monthLabel === monthLabel && d.clientName === client)
            clientGroups[client].push(item?.deliveryCount || 0)
          })
        })
        series = selectedClients.value.map((clientName, index) => ({
          name: clientName,
          type: 'line',
          smooth: false,
          symbol: 'circle',
          symbolSize: 16,
          lineStyle: { width: 8 },
          itemStyle: { color: colors[index % colors.length] },
          label: {
            show: true,
            position: 'top',
            formatter: '{c}',
            fontSize: 12,
            fontWeight: 'bold',
            color: colors[index % colors.length],
            offset: [0, -5],
          },
          data: clientGroups[clientName],
        }))
      }
    }

    const option = {
      title: {
        text: `${selectedOrderType.value}工单统计趋势`,
        left: 'center',
        textStyle: {
          fontSize: 16,
          fontWeight: 'bold',
        },
      },
      tooltip: {
        trigger: 'axis',
        axisPointer: {
          type: 'cross',
          label: {
            backgroundColor: '#6a7985',
          },
        },
        formatter: (params: any[]) => {
          let result = `<div style="font-weight: bold; margin-bottom: 5px;">${params[0].axisValue}</div>`
          params.forEach((param) => {
            result += `<div style="display: flex; align-items: center; margin: 2px 0;">
              <span style="display: inline-block; width: 10px; height: 10px; background-color: ${param.color}; border-radius: 50%; margin-right: 5px;"></span>
              ${param.seriesName}: ${param.value}个工单
            </div>`
          })
          return result
        },
      },
      legend: {
        top: 30,
        type: 'scroll',
      },
      grid: {
        left: '3%',
        right: '4%',
        bottom: '3%',
        top: 80,
        containLabel: true,
      },
      xAxis: {
        type: 'category',
        boundaryGap: false,
        data: xAxisData,
        axisLabel: {
          rotate: selectedTimePeriod.value === '6months' ? 45 : 0,
        },
      },
      yAxis: {
        type: 'value',
        name: '工单数量',
        minInterval: 1,
        axisLabel: {
          formatter: '{value}个',
        },
      },
      series,
    }

    chartInstance.value.setOption(option)
    // 新增：监听点击事件
    chartInstance.value.on('click', (params: any) => {
      // 只有交付发送工单类型时，点击客户曲线（series）才跳转
      if (
        selectedOrderType.value === '交付发送' && params.componentType === 'series' && params.seriesName
      ) {
        router.push({
          path: '/client_manage/send_detail',
          query: {
            clientName: params.seriesName,
          },
        })
      }
    })
  }
  catch (error) {
    console.error('渲染图表时出错:', error)
  }
}

// 获取客户列表数据
async function fetchClients() {
  clientLoading.value = true
  try {
    const res = await deliveryApi.getClientList()

    if (res?.data) {
      const mappedData: Client[] = (res.data.list || []).map((client: any) => ({
        clientName: client.clientName || '', // 客户名称
        dongleCount: client.dongleCount || 0,
        modelCount: client.modelCount || 0,
        modelVersionCount: client.modelVersionCount || 0,
        clientinfo: client.clientinfo || '',
        licenseStats: {
          validCount: client.licenseStats?.validCount || 0,
          expiringCount: client.licenseStats?.expiringCount || 0,
          expiredCount: client.licenseStats?.expiredCount || 0,
        },
      }))

      clientList.value = mappedData

      selectedClients.value = mappedData.map(client => client.clientName)
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

// 调用接口函数
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

// 工具函数
function getTotalLicenseCount(client: Client): number {
  return client.licenseStats.validCount + client.licenseStats.expiringCount + client.licenseStats.expiredCount
}

const expiringClientsInfo = computed(() => {
  const expiringClients = clientList.value.filter(client => client.licenseStats.expiringCount > 0)
  return expiringClients.map(client => ({
    name: client.clientName,
    count: client.licenseStats.expiringCount,
  }))
})

function hasExpiringLicense(client: Client): boolean {
  return client.licenseStats.expiringCount > 0
}

// 切换时间范围
function changeTimePeriod(period: '7days' | '1month' | '6months') {
  selectedTimePeriod.value = period
  renderChart() // 直接重新渲染，不需要重新获取数据
}

// 切换工单类型
function changeOrderType(type: '版本迭代' | '交付发送') {
  selectedOrderType.value = type
  renderChart() // 直接重新渲染，不需要重新获取数据
}

// 切换客户选择
function toggleClient(clientName: string) {
  const index = selectedClients.value.indexOf(clientName)
  if (index > -1) {
    selectedClients.value.splice(index, 1)
  }
  else {
    selectedClients.value.push(clientName)
  }
  renderChart() // 直接重新渲染，不需要重新获取数据
}

function isCurrentStep(order: OrderItem, step: string): boolean {
  if (['待封装', '待加密', '待发送'].includes(step)) {
    return order.status === '进行中' && order.statusTodo === step
  }
  return order.status === step
}

function getOrderSteps(type: string): string[] {
  if (type === '版本迭代') {
    return ['待审批', '待分发', '待封装', '已完成']
  }
  if (type === '交付发送') {
    return ['待审批', '待分发', '待加密', '待发送', '已完成']
  }
  if (type === '版本迭代+交付发送') {
    return ['待审批', '待分发', '待封装', '待加密', '待发送', '已完成']
  }
  // 问题复现、功能开发、其他
  return ['待审批', '待分发', '进行中', '已完成']
}

function getStepColor(order: OrderItem, step: string): string {
  const steps = getOrderSteps(order.type)
  const stepIdx = steps.indexOf(step)

  // 当前流程索引
  let currentIdx = -1

  // 1. 如果流程已到“进行中”及之后（即 status 为“进行中”或“已完成”），
  //    则用 statusTodo 判断“待封装/待加密/待发送”，用 status 判断其他
  if (order.status === '进行中') {
    // statusTodo 只可能是“待封装/待加密/待发送”
    currentIdx = steps.indexOf(order.statusTodo ?? '')
    // 如果 statusTodo 未进入流程，则 currentIdx 取 status
    if (currentIdx === -1) {
      currentIdx = steps.indexOf(order.status)
    }
  }
  else {
    // status 为“待审批/待分发/已完成”等
    currentIdx = steps.indexOf(order.status)
  }

  // 已完成的步骤绿色
  if (stepIdx < currentIdx) {
    return '#67c23a'
  }
  // 当前步骤蓝色
  if (stepIdx === currentIdx) {
    return '#409eff'
  }
  // 未到灰色
  return '#c0c4cc'
}

const pageLoading = ref(false)
// 获取与我相关的工单
async function fetchMyRelatedOrders() {
  myRelatedOrdersLoading.value = true
  try {
    myRelatedOrders.value = await apiFetchMyRelatedOrders()
  }
  catch (error) {
    console.error('获取与我相关的工单失败:', error)
  }
  finally {
    myRelatedOrdersLoading.value = false
  }
}

// 页面加载时获取数据
onMounted(async () => {
  pageLoading.value = true
  try {
    await fetchUserOrders() // 获取待办工单
    await fetchRecentOrders() // 获取近期发送工单
    await fetchMyRelatedOrders() // 获取与我相关的工单
    await fetchClients() // 获取客户列表
    await fetchChartData() // 获取图表数据
    renderChart() // 渲染图表
  }
  finally {
    pageLoading.value = false
  }
})

// 新增：组件卸载时清理
onUnmounted(() => {
  if (chartInstance.value) {
    chartInstance.value.dispose()
  }
})
</script>

<template>
  <div v-loading="pageLoading" class="relative min-h-screen">
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

      <!-- 待办工单内容块，仅显示流程信息，使用时间轴 -->
      <FaPageMain class="mb-6">
        <template #title>
          <div class="flex items-center gap-2">
            <i class="i-mdi-clipboard-list-outline text-2xl text-blue-500" />
            <span class="text-xl font-bold">我的待办工单</span>
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

          <template #default>
            <div v-if="userOrders.length === 0" class="py-8 text-center text-gray-500">
              <i class="i-mdi-clipboard-outline mb-2 text-4xl" />
              <p>暂无待办工单</p>
            </div>

            <div v-else class="space-y-4">
              <FaPageMain
                v-for="order in userOrders"
                :key="order.orderID"
                class="border border-gray-200"
              >
                <div class="px-4 py-2">
                  <div class="mb-2 text-lg font-bold text-black">
                    工单#{{ order.orderID }}
                    <span class="ml-2 text-base font-semibold" :class="{
                      'text-blue-700': order.type === '问题复现',
                      'text-green-700': order.type === '版本迭代',
                      'text-yellow-700': order.type === '交付发送',
                      'text-purple-700': order.type === '版本迭代+交付发送',
                      'text-pink-700': order.type === '功能开发',
                      'text-gray-700': order.type === '其他',
                    }">{{ order.type }}</span>
                  </div>
                  <!-- 新增：工单详细信息 -->
                  <div class="mt-2 flex flex-wrap items-center gap-6 text-sm">
                    <span>
                      <i class="i-mdi-cube mr-1 text-blue-400" />
                      <span class="text-gray-600">模型：</span>
                      <span class="text-black font-bold">{{ order.modelID }}</span>
                    </span>
                    <span>
                      <i class="i-mdi-account mr-1 text-blue-400" />
                      <span class="text-gray-600">发起人：</span>
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
                  </div>
                  <!-- 横向时间轴 -->
                  <div class="flex items-center justify-start gap-8 px-2 py-4">
                    <template v-for="(step, idx) in getOrderSteps(order.type)" :key="step">
                      <div class="flex flex-col items-center">
                        <div
                          class="w-8 h-8 flex items-center justify-center rounded-full border-2"
                          :class="isCurrentStep(order, step)
                            ? 'bg-blue-100 border-blue-600 text-blue-600 font-bold'
                            : getStepColor(order, step) === '#67c23a'
                              ? 'bg-green-100 border-green-600 text-green-600'
                              : 'bg-gray-100 border-gray-300 text-gray-400'"
                        >
                          <span v-if="isCurrentStep(order, step)">✔</span>
                          <span v-else>{{ idx + 1 }}</span>
                        </div>
                        <span
                          class="mt-2 text-sm"
                          :class="isCurrentStep(order, step)
                            ? 'font-bold text-blue-600'
                            : getStepColor(order, step) === '#67c23a'
                              ? 'text-green-600'
                              : 'text-gray-500'"
                        >
                          {{ step }}
                        </span>
                      </div>
                      <div v-if="idx < getOrderSteps(order.type).length - 1" class="flex-1 h-1 bg-gray-300 mx-2"></div>
                    </template>
                  </div>
                </div>
              </FaPageMain>
            </div>
          </template>
        </el-skeleton>
      </FaPageMain>

      <!-- 与我相关的工单模块 -->
      <FaPageMain class="mb-6">
        <template #title>
          <div class="flex items-center gap-2">
            <i class="i-mdi-account-outline text-2xl text-green-500" />
            <span class="text-xl font-bold">与我相关的工单</span>
            <span class="flex items-center gap-1 text-base text-gray-500 font-medium">
              <i class="i-mdi-information-outline text-green-500" />
              (当前显示前{{ Math.min(3, myRelatedOrders.length) }}条与我相关的工单，共{{ myRelatedOrders.length }}条)
            </span>
          </div>
        </template>

        <el-skeleton :loading="myRelatedOrdersLoading" animated>
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

          <template #default>
            <div v-if="myRelatedOrders.length === 0" class="py-8 text-center text-gray-500">
              <i class="i-mdi-clipboard-outline mb-2 text-4xl" />
              <p>暂无与我相关的工单</p>
            </div>

            <div v-else class="space-y-4">
              <FaPageMain
                v-for="order in myRelatedOrders"
                :key="order.orderID"
                class="border border-gray-200"
              >
                <div class="px-4 py-2">
                  <div class="mb-2 text-lg font-bold text-black">
                    工单#{{ order.orderID }}
                    <span class="ml-2 text-base font-semibold" :class="{
                      'text-blue-700': order.type === '问题复现',
                      'text-green-700': order.type === '版本迭代',
                      'text-yellow-700': order.type === '交付发送',
                      'text-purple-700': order.type === '版本迭代+交付发送',
                      'text-pink-700': order.type === '功能开发',
                      'text-gray-700': order.type === '其他',
                    }">{{ order.type }}</span>
                  </div>
                  <!-- 新增：工单详细信息 -->
                  <div class="mt-2 flex flex-wrap items-center gap-6 text-sm">
                    <span>
                      <i class="i-mdi-cube mr-1 text-blue-400" />
                      <span class="text-gray-600">模型：</span>
                      <span class="text-black font-bold">{{ order.modelID }}</span>
                    </span>
                    <span>
                      <i class="i-mdi-account mr-1 text-blue-400" />
                      <span class="text-gray-600">发起人：</span>
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
                  </div>
                  <!-- 横向时间轴 -->
                  <div class="flex items-center justify-start gap-8 px-2 py-4">
                    <template v-for="(step, idx) in getOrderSteps(order.type)" :key="step">
                      <div class="flex flex-col items-center">
                        <div
                          class="w-8 h-8 flex items-center justify-center rounded-full border-2"
                          :class="isCurrentStep(order, step)
                            ? 'bg-blue-100 border-blue-600 text-blue-600 font-bold'
                            : getStepColor(order, step) === '#67c23a'
                              ? 'bg-green-100 border-green-600 text-green-600'
                              : 'bg-gray-100 border-gray-300 text-gray-400'"
                        >
                          <span v-if="isCurrentStep(order, step)">✔</span>
                          <span v-else>{{ idx + 1 }}</span>
                        </div>
                        <span
                          class="mt-2 text-sm"
                          :class="isCurrentStep(order, step)
                            ? 'font-bold text-blue-600'
                            : getStepColor(order, step) === '#67c23a'
                              ? 'text-green-600'
                              : 'text-gray-500'"
                        >
                          {{ step }}
                        </span>
                      </div>
                      <div v-if="idx < getOrderSteps(order.type).length - 1" class="flex-1 h-1 bg-gray-300 mx-2"></div>
                    </template>
                  </div>
                </div>
              </FaPageMain>
            </div>
          </template>
        </el-skeleton>
      </FaPageMain>

      <!-- 新增：工单统计图表模块 -->
      <FaAuth value="SuperUser">
        <FaPageMain class="mb-6">
          <template #title>
            <div class="flex items-center gap-2">
              <i class="i-mdi-chart-line text-2xl text-purple-500" />
              <span class="text-xl font-bold">工单统计分析</span>
              <span class="flex items-center gap-1 text-base text-gray-500 font-medium">
                <i class="i-mdi-information-outline text-purple-500" />
                (显示不同时间段的工单发送趋势)
              </span>
            </div>
            <!-- 新增提示语 -->
            <div v-if="selectedOrderType === '交付发送'" class="mt-2 text-sm text-orange-600 font-semibold">
              提示：点击客户曲线数据点可跳转到该客户的发送详情页面
            </div>
          </template>

          <!-- 控制按钮区域 -->
          <div class="mb-4 flex flex-wrap items-center gap-4">
            <!-- 时间范围选择 -->
            <div class="flex items-center gap-2">
              <span class="text-sm text-gray-600 font-medium">时间范围：</span>
              <el-button-group>
                <el-button
                  v-for="option in timePeriodOptions"
                  :key="option.value"
                  :type="selectedTimePeriod === option.value ? 'primary' : 'default'"
                  size="small"
                  @click="changeTimePeriod(option.value)"
                >
                  {{ option.label }}
                </el-button>
              </el-button-group>
            </div>

            <!-- 工单类型选择 -->
            <div class="flex items-center gap-2">
              <span class="text-sm text-gray-600 font-medium">工单类型：</span>
              <el-button-group>
                <el-button
                  v-for="option in orderTypeOptions"
                  :key="option.value"
                  :type="selectedOrderType === option.value ? 'primary' : 'default'"
                  size="small"
                  @click="changeOrderType(option.value)"
                >
                  {{ option.label }}
                </el-button>
              </el-button-group>
            </div>

            <!-- 新增：客户选择 -->
            <div class="flex items-center gap-2">
              <span class="text-sm text-gray-600 font-medium">显示客户：</span>
              <div class="flex items-center gap-3">
                <el-checkbox
                  v-for="client in availableClients"
                  :key="client"
                  :model-value="selectedClients.includes(client)"
                  @change="toggleClient(client)"
                >
                  {{ client }}
                </el-checkbox>
              </div>
            </div>
          </div>

          <!-- 图表容器 -->
          <el-skeleton :loading="chartLoading" animated>
            <template #template>
              <div class="h-96">
                <el-skeleton-item variant="rect" style="width: 100%; height: 100%;" />
              </div>
            </template>

            <template #default>
              <div
                ref="chartContainer"
                class="h-96 w-full"
                style="min-height: 400px;"
              />
            </template>
          </el-skeleton>
        </FaPageMain>
      </FaAuth>

      <!-- 近期发送工单信息内容块 -->
      <FaAuth :value="['SuperUser', 'ModelEngineer']">
        <FaPageMain class="mb-6" v-if="recentOrders.length > 0">
          <template #title>
            <div class="flex items-center gap-2">
              <i class="i-mdi-send-outline text-2xl text-green-500" />
              <span class="text-xl font-bold">我近期发送的工单</span>
              <span class="flex items-center gap-1 text-base text-gray-500 font-medium">
                <i class="i-mdi-information-outline text-green-500" />
                (显示最近{{ recentOrders.length }}条发送的工单)
              </span>
            </div>
          </template>

          <!-- ...近期发送工单内容保持不变... -->
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
      </FaAuth>

      <!-- 客户授权统计模块 -->
      <FaAuth value="SoftwareEngineer">
        <FaPageMain>
          <template #title>
            <div class="flex items-center gap-2">
              <i class="i-mdi-shield-key-outline text-2xl text-orange-500" />
              <span class="text-xl font-bold">客户授权统计</span>
              <span class="flex items-center gap-1 text-base text-gray-500 font-medium">
                <i class="i-mdi-information-outline text-orange-500" />
                (显示所有客户的授权信息概览)
              </span>
            </div>
          </template>

          <!-- ...客户授权统计内容保持不变... -->
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
                <!-- 临期授权提醒信息 -->
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
                  <!-- 客户名称行 -->
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
                    <!-- 临期授权标识 -->
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
      </FaAuth>
    </div>
  </div>
</template>

<style scoped>
.el-badge {
  --el-badge-bg-color: #409eff;
}
</style>
