<route lang="yaml">
meta:
  title:
  # enabled: false
</route>

<script setup lang="ts">
import { ElLoading, ElMessage } from 'element-plus'
import { onMounted, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import * as XLSX from 'xlsx'
import modelApi from '@/api/modules/model'

// ----------------定义数据结构--------------------
const loading = ref(false) // loading变量定义
// 动态数据：模型列表，每个模型有多个工单
const modelList = ref<Model[]>([])
const route = useRoute() // 获取当前路由信息
const router = useRouter()

// 当前选中的模型
const currentModel = ref<any>(null)
const currentFunctionOrders = ref<Order[]>([])
const currentReproduceOrders = ref<Order[]>([])
const currentDeliveryOrders = ref<Order[]>([])
const currentCoordinationOrders = ref<Order[]>([])

// 工单类型接口，描述了一个工单可能包含的所有字段，工单完成类型
interface Order {
  // ------------------通用字段------------------------
  orderID: string // 关联工单ID
  // type: 'function' | 'reproduce' | 'delivery' | 'iteration' // 工单类型: 功能开发 | 问题复现 | 交付发送 | 版本迭代
  type: '问题复现' | '版本迭代' | '交付发送' | '直接封装+发送' | '功能开发' | '其他'
  status?: string // 工单状态
  startTime?: string // 发起时间
  promoterID?: string // 发起人ID
  coordinationID?: string // 协调单ID
  baseModelVersion?: string // 模型父版本
  finishTime?: string // 完成时间
  executorID?: string // 执行人ID

  // ----------完成工单相关字段------------------------
  finishRemark?: string // 备注
  // ------------------问题复现类---------------------
  finishPhenomenon?: string // 复现现象
  // ------------------版本迭代类---------------------
  updateNotes?: string // 更新内容
  packingRequirement?: string // 封装要求
  apiChanged?: boolean // 是否有接口变化
  finishModelVersion?: string // 升级后模型版本ID（可选，主要用于版本迭代类工单）
  // ------------------交付发送类---------------------
  isCAEChecked?: boolean // 是否通过CAE平台验证
  isEncrypted?: boolean // 是否加密
  targetCustomer?: string // 目标客户
  finishAuthId?: string // 授权ID
  finishShellNo?: string // 外壳号
  // ------------------功能开发类---------------------
  finishFeatureDesc?: string // 功能描述
  finishModelVersionId?: string // 升级后模型版本ID（可选，主要用于功能开发类工单）
  // ------------------其他---------------------
  finishRemarkOther?: string // 内容描述（主要用于其他类型工单，作为预留接口）

}

// 模型类型接口，描述了一个模型的基本信息和包含的工单列表
interface Model {
  modelID: string // 模型ID
  version: string // 模型版本号
  updatetime: string // 模型更新时间
  orders: Order[] // 工单列表
}

const showFunctionDialog = ref(false) // 控制封装功能弹窗显示
const showReproduceDialog = ref(false) // 控制复现弹窗显示
const showCoordinationDialog = ref(false) // 控制协调单弹窗显示
const showClientDialog = ref(false) // 控制客户详细信息弹窗显示
const modalConfig = ref({
  maximizable: false,
  closable: true,
  draggable: false,
  center: false,
  header: true,
  footer: true,
  contentHeight: '',
  loading: false,
})

// 分页相关响应式变量
const page = ref(1) // 当前页码
const pageSize = ref(10) // 每页显示数量
const total = ref(0) // 总数据量

// 一键展开/收起功能
const expandedMap = ref<Record<string, boolean>>({})
// -----------------点击按钮进行对应的工单筛选----------------------
// 功能开发类工单
function openFunctionDialog(model: Model) {
  currentModel.value = model
  currentFunctionOrders.value = model.orders.filter(o => o.type === '功能开发')
  showFunctionDialog.value = true
}

// 问题复现类工单
function openReproduceDialog(model: Model) {
  currentModel.value = model
  currentReproduceOrders.value = model.orders.filter(o => o.type === '问题复现')
  showReproduceDialog.value = true
}

// 交付发送类工单
function openClientDialog(model: Model) {
  currentModel.value = model
  currentDeliveryOrders.value = model.orders.filter(o => o.type === '交付发送')
  showClientDialog.value = true
}

// 协调单显示
function openCoordinationOrderDialog(model: Model) {
  currentModel.value = model
  currentCoordinationOrders.value = model.orders.filter(o => o.coordinationID)
  showCoordinationDialog.value = true
}

function handleOrderNumberClick(orderId: string) {
  console.warn('跳转到工单详情:', orderId)

  // 跳转到工单列表页面，传递工单号参数
  router.push({
    path: '/order_list',
    query: {
      orderId,
      fromSendDetail: 'true', // 标识来源，便于区分加载逻辑
    },
  })
}

// -------------后端事件处理函数---------------
// 预处理工单列表，将版本迭代+交付发送拆分为两个工单
function preprocessOrders(list: Model[]): Model[] {
  return list.map((model) => {
    const newOrders: Order[] = []
    model.orders.forEach((order) => {
      if (order.type === '直接封装+发送') {
        // 版本迭代工单，保留协调单信息
        newOrders.push({
          ...order,
          type: '版本迭代',
        })
        // 交付发送工单，去除协调单信息
        const { coordinationID, ...rest } = order
        newOrders.push({
          ...rest,
          type: '交付发送',
          coordinationID: undefined,
        })
      }
      else {
        newOrders.push(order)
      }
    })
    return { ...model, orders: newOrders }
  })
}

// 获取模型列表及工单信息，带分页参数
async function apiFetchUserModels() {
  const modelRouteName = route.matched[0].name
  const params = {
    page: page.value,
    pageSize: pageSize.value,
    modelID: modelRouteName ? String(modelRouteName) : '',
  }
  const res = await modelApi.getModelList(params)
  console.warn('获取模型列表数据:', res)
  if (res?.data) {
    // 数据映射处理
    const mappedData = {
      list: (res.data.list || []).map((model: any) => ({
        modelID: model.modelID || model.model || '', // 模型ID
        version: model.version || '', // 模型版本号
        updatetime: model.updateTime || '', // 模型更新时间
        orders: (model.orders || []).map((order: any) => ({
          orderID: order.orderID || '',
          type: order.ticketType || '其他', // 工单类型
          status: order.status || '', // 工单状态
          startTime: order.startTime || order.createTime || '', // 发起时间
          promoterID: order.promoterID || order.creatorId || '', // 发起人ID
          coordinationID: order.coordinationID || '', // 协调单
          baseModelVersion: order.modelVersion || order.mv_id || '', // 模型基准版本
          finishTime: order.completedTime || '', // 完成时间
          executorID: order.executorID || '',
          finishRemark: order.remark || '', // 完成备注
          finishPhenomenon: order.finishPhenomenon || order.phenomenon || '', // 复现现象
          updateNotes: order.updateNote || '', // 更新内容
          packingRequirement: order.packRequirement || '', // 封装要求
          apiChanged: order.interfaceChanged === '1', // 接口变化
          finishModelVersion: order.newModelVersion || order.version || '', // 升级后模型版本
          isCAEChecked: order.validatedByCae === '1', // CAE验证
          isEncrypted: order.encrypted === '1', // 是否加密
          targetCustomer: order.targetClient || '', // 目标客户
          finishAuthId: order.license || '', // 授权ID
          finishShellNo: order.dongle || '', // 外壳号
          finishFeatureDesc: order.featureFinal || order.featureInit || '', // 功能描述
          finishModelVersionId: order.newModelVersion || '', // 升级后模型版本ID（功能开发类）
          finishRemarkOther: order.description || '', // 其他备注
        })),
      })),
      total: res.data.total || 0,
    }

    console.warn('获取模型列表数据:', mappedData)
    return mappedData
  }
  console.warn('获取模型列表数据失败，返回默认空数据')
  return { list: [], total: 0 }
}

// --------------模型版本信息导出弹窗----------------
interface ModelVersionInfo {
  version: string // 模型版本号
  updateNotes: string // 更新内容
}
const versionDialogVisible = ref(false)
const versionTableData = ref<ModelVersionInfo[]>([])
const selectedVersions = ref<ModelVersionInfo[]>([])
const versionDialogLoading = ref(false)
function openVersionDialog() {
  versionDialogVisible.value = true
}

// 后端获取模型版本信息
async function fetchVersionTableData() {
  versionDialogLoading.value = true
  try {
    console.warn('获取模型版本数据:', route.matched[0])
    let modelID
    if (route.matched[0].name === 'ATA21_30_36_52_ECS') {
      modelID = 'ATA21/30/36/52_ECS'
    }
    else {
      modelID = route.matched[0].name
    }
    const res = await modelApi.getVersionList({ modelID: modelID ? String(modelID) : '' })
    // 手动映射，确保每项都符合 ModelVersionInfo 类型
    versionTableData.value = (res?.data?.list || []).map((item: any) => ({
      version: item.version ?? '', // 模型版本号
      updateNotes: item.updateNotes ?? '', // 更新内容
    }))
  }
  catch (e) {
    ElMessage.error('获取模型版本数据失败')
    console.error('获取模型版本数据失败:', e)
    versionTableData.value = []
  }
  finally {
    versionDialogLoading.value = false
  }
}

function handleSelectionChange(selection: any[]) {
  selectedVersions.value = selection
}

function getBeijingTimestamp() {
  const now = new Date()
  // 北京时间 = UTC时间 + 8小时
  const bjTime = new Date(now.getTime() + 8 * 60 * 60 * 1000)
  const yyyy = bjTime.getUTCFullYear()
  const MM = String(bjTime.getUTCMonth() + 1).padStart(2, '0')
  const dd = String(bjTime.getUTCDate()).padStart(2, '0')
  const HH = String(bjTime.getUTCHours()).padStart(2, '0')
  const mm = String(bjTime.getUTCMinutes()).padStart(2, '0')
  const ss = String(bjTime.getUTCSeconds()).padStart(2, '0')
  return `${yyyy}${MM}${dd}${HH}${mm}${ss}`
}

function submitSelectedVersions() {
  if (selectedVersions.value.length === 0) {
    ElMessage.warning('请先选择要导出的模型版本')
    return
  }
  const ataName = route.matched[0].name ? String(route.matched[0].name) : '模型版本'
  const timestamp = getBeijingTimestamp()

  // 1. 按前三位分组并合并内容
  const grouped: Record<string, string[]> = {}
  selectedVersions.value.forEach((item) => {
    const match = item.version.match(/^(\d+\.\d+\.\d+)/)
    const key = match ? match[1] : item.version
    if (!grouped[key]) {
      grouped[key] = []
    }
    grouped[key].push(item.updateNotes)
  })

  // 2. 构造二维数组数据（表头+内容）
  const aoa: any[][] = [['序号', '模型版本', '更新内容']]
  const merges: any[] = []
  let rowIdx = 1
  let seq = 1
  Object.entries(grouped).forEach(([version, notes]) => {
    if (notes.length === 1) {
      aoa.push([seq, version, notes[0]])
      rowIdx++
    }
    else {
      // 多条内容，模型版本单元格合并
      aoa.push([seq, version, notes[0]])
      for (let i = 1; i < notes.length; i++) {
        aoa.push(['', '', notes[i]])
      }
      // 合并模型版本单元格（B列），序号单元格（A列）
      merges.push({ s: { r: rowIdx, c: 1 }, e: { r: rowIdx + notes.length - 1, c: 1 } })
      merges.push({ s: { r: rowIdx, c: 0 }, e: { r: rowIdx + notes.length - 1, c: 0 } })
      rowIdx += notes.length
    }
    seq++
  })

  // 3. 导出Excel
  const worksheet = XLSX.utils.aoa_to_sheet(aoa)
  worksheet['!merges'] = merges
  const workbook = XLSX.utils.book_new()
  XLSX.utils.book_append_sheet(workbook, worksheet, ataName)
  XLSX.writeFile(workbook, `${ataName}_updateNotes_${timestamp}.xlsx`)
  ElMessage.success('Excel导出成功')
  versionDialogVisible.value = false
}
const globalLoading = ref(false)
let loadingInstance: any = null

// 加载模型数据并处理 loading 状态
async function fetchUserModels() {
  globalLoading.value = true
  loadingInstance = ElLoading.service({
    lock: true,
    text: '加载中，请稍候...',
    background: 'rgba(255,255,255,0.7)',
    customClass: 'custom-global-loading',
  })
  try {
    const result = await apiFetchUserModels()
    modelList.value = preprocessOrders(result.list || [])
    total.value = result.total || 0
  }
  finally {
    globalLoading.value = false
    loadingInstance.close()
  }
}

// 新增：退出全局 loading 按钮处理
function exitGlobalLoading() {
  globalLoading.value = false
  if (loadingInstance) {
    loadingInstance.close()
  }
}

// 分页切换事件处理函数
function handlePageChange(newPage: number) {
  page.value = newPage // 更新当前页码
  fetchUserModels() // 重新加载数据
}

// 页面初始化时加载第一页数据
onMounted(() => {
  fetchUserModels()
})

// 控制每个模型版本的展开/收起
function expandModel(version: string, expand: boolean) {
  expandedMap.value[version] = expand
}
</script>

<template>
  <div v-loading="loading" class="min-h-screen">
    <!-- 退出按钮，仅在 loading 时显示 -->
    <button
      v-if="globalLoading"
      class="global-loading-exit-btn el-button el-button--danger"
      style="position: fixed; top: 60px; left: 50%; z-index: 10001; transform: translateX(-50%);"
      @click="exitGlobalLoading"
    >
      退出加载
    </button>
    <el-skeleton :loading="loading" animated>
      <template #default>
        <div>
          <el-skeleton :loading="loading" animated>
            <template #default>
              <!-- 顶部标题栏 -->
              <div class="mb-6 border border-gray-200 rounded-lg bg-white p-6 shadow-sm">
                <div class="flex items-center justify-between">
                  <div class="flex items-center">
                    <h1 class="text-2xl text-gray-800 font-bold">
                      {{ route.matched[0].name }} 模型版本列表
                    </h1>
                  </div>
                  <!-- 新增：右侧按钮 -->
                  <el-button type="primary" @click="openVersionDialog">
                    导出更新内容
                  </el-button>
                </div>

                <!-- 文字提示行 -->
                <div class="mt-3 text-sm text-gray-600">
                  本页面展示 {{ route.matched[0].name }} 模型的所有版本信息及相关工单详情
                </div>
              </div>
              <!-- 动态渲染所有模型版本 -->
              <FaPageMain
                v-for="model in modelList"
                :key="model.version"
                :title="`模型版本 ${model.version}`"
                :collaspe="!expandedMap[model.version]"
                height="auto"
                class="w-full"
              >
                <template #title>
                  <div class="flex items-center gap-2">
                    <el-tooltip
                      :content="model.orders.some(o => o.type === '版本迭代' && o.apiChanged) ? '接口与父版本有变化' : '接口与父版本无变化'"
                      placement="top"
                    >
                      <FaIcon
                        :name="model.orders.some(o => o.type === '版本迭代' && o.apiChanged) ? 'error_state' : 'ready_state'"
                        :class="model.orders.some(o => o.type === '版本迭代' && o.apiChanged) ? 'size-8 text-red-500' : 'size-8 text-green-500'"
                      />
                    </el-tooltip>
                    <span class="ml-2 text-2xl font-bold">
                      {{ model.version }}
                      <sup
                        class="ml-1 text-sm font-bold"
                        :class="model.orders.some(o => o.type === '交付发送' && o.isCAEChecked) ? 'text-green-700' : 'text-gray-400'"
                        style="line-height: 1;"
                      >
                        CAE
                        <FaIcon
                          v-if="model.orders.some(o => o.type === '交付发送' && o.isCAEChecked)"
                          name="check"
                          class="size-4 text-green-600"
                          style="vertical-align: middle;"
                        />
                      </sup>
                    </span>
                    <div class="ml-auto flex gap-2">
                      <FaButton
                        class="rounded px-5 py-1.5 font-semibold transition-colors duration-150"
                        :class="[
                          model.orders.some(o => o.type === '功能开发')
                            ? 'bg-cyan-400 text-black border border-cyan-600 hover:bg-cyan-500'
                            : 'bg-gray-300 text-gray-400 border border-gray-400 cursor-not-allowed',
                        ]"
                        :disabled="!model.orders.some(o => o.type === '功能开发')"
                        @click="openFunctionDialog(model)"
                      >
                        封装功能
                      </FaButton>
                      <FaButton
                        class="rounded px-5 py-1.5 font-semibold transition-colors duration-150"
                        :class="[
                          model.orders.some(o => o.type === '问题复现')
                            ? 'bg-purple-300 text-black border border-purple-500 hover:bg-purple-400'
                            : 'bg-gray-300 text-gray-400 border border-gray-400 cursor-not-allowed',
                        ]"
                        :disabled="!model.orders.some(o => o.type === '问题复现')"
                        @click="openReproduceDialog(model)"
                      >
                        复现
                      </FaButton>
                      <FaButton
                        class="rounded px-5 py-1.5 font-semibold transition-colors duration-150"
                        :class="[
                          model.orders.some(o => o.coordinationID)
                            ? 'bg-blue-300 text-black border border-blue-500 hover:bg-blue-400'
                            : 'bg-gray-300 text-gray-400 border border-gray-400 cursor-not-allowed',
                        ]"
                        :disabled="!model.orders.some(o => o.coordinationID)"
                        @click="openCoordinationOrderDialog(model)"
                      >
                        协调单
                      </FaButton>
                      <FaButton
                        class="rounded px-5 py-1.5 font-semibold transition-colors duration-150"
                        :class="[
                          model.orders.some(o => o.type === '交付发送')
                            ? 'bg-yellow-300 text-black border border-yellow-500 hover:bg-yellow-400'
                            : 'bg-gray-300 text-gray-400 border border-gray-400 cursor-not-allowed',
                        ]"
                        :disabled="!model.orders.some(o => o.type === '交付发送')"
                        @click="openClientDialog(model)"
                      >
                        已发送
                      </FaButton>
                    </div>
                  </div>
                  <!-- 新增一行，放模型版本号和更新时间 -->
                  <div class="ml-2 mt-2 flex items-center gap-2">
                    <FaIcon
                      name="expand"
                      class="mr-2 cursor-pointer text-xl"
                      @click.stop="expandModel(model.version, !expandedMap[model.version])"
                    />
                    <span class="text-base text-black">ATA章节号：{{ route.matched[0].name }}</span>
                    <FaIcon name="updatetime" class="mx-1 size-6" />
                    <span class="text-base text-black">更新时间：{{ model.updatetime }}</span>
                  </div>
                </template>
                <!-- 动态渲染该模型下所有版本迭代类工单 -->
                <div class="pl-4 space-y-4">
                  <FaPageMain
                    v-for="order in model.orders.filter(o => o.type === '版本迭代')"
                    :key="order.orderID"
                    :title="`工单#${order.orderID}`"
                    :collaspe="!expandedMap[model.version]"
                    height="auto"
                    class="w-full"
                  >
                    <template #title>
                      <div class="flex items-center text-lg text-black font-bold">
                        工单#{{ order.orderID }}
                        <span
                          class="ml-4 align-middle text-base text-green-700 font-bold"
                        >
                          {{ order.type }}
                        </span>
                      </div>
                    </template>
                    <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4">
                      <!-- 发起人 -->
                      <div class="col-span-1 w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">发起人：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                          :value="order.promoterID"
                          readonly
                        >
                      </div>
                      <!-- 发起时间 -->
                      <div class="col-span-1 w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">发起时间：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                          :value="order.startTime"
                          readonly
                        >
                      </div>
                      <!-- 相应协调单 -->
                      <div class="col-span-1 w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">协调单ID：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                          :value="order.coordinationID || ''"
                          readonly
                        >
                      </div>
                      <!-- 接口变化 -->
                      <div class="col-span-1 w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">
                          接口与{{ order.baseModelVersion || '基准版本' }}是否变化：
                        </span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                          :value="order.apiChanged ? '是' : '否'"
                          readonly
                        >
                      </div>
                      <!-- 更新内容 -->
                      <div class="col-span-1 w-full flex items-start gap-2">
                        <span class="w-32 text-black font-semibold">更新内容：</span>
                        <textarea
                          class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold leading-snug"
                          :value="order.updateNotes"
                          rows="2"
                          readonly
                        />
                      </div>
                      <!-- 封装要求 -->
                      <div class="col-span-1 w-full flex items-start gap-2">
                        <span class="w-32 text-black font-semibold">封装要求：</span>
                        <textarea
                          class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold leading-snug"
                          :value="order.packingRequirement"
                          rows="2"
                          readonly
                        />
                      </div>
                      <!-- 完成人 -->
                      <div class="col-span-1 w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">执行人：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                          :value="order.executorID"
                          readonly
                        >
                      </div>
                      <!-- 完成时间 -->
                      <div class="col-span-1 w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">完成时间：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                          :value="order.finishTime"
                          readonly
                        >
                      </div>
                      <!-- 基准版本 -->
                      <div class="flex items-start gap-2">
                        <span class="w-32 text-black font-semibold">基准版本</span>
                        <input
                          class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                          :value="order.baseModelVersion"
                          readonly
                        >
                      </div>
                      <!-- 完成备注 -->
                      <div class="flex items-start gap-2">
                        <span class="w-32 text-black font-semibold">完成备注</span>
                        <input
                          class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                          :value="order.finishRemark"
                          readonly
                        >
                      </div>
                    </div>
                    <div class="mt-6" />
                  </FaPageMain>
                </div>
                <div class="mt-10" />
              </FaPageMain>

              <FaModal
                v-model="showFunctionDialog"
                title="封装功能"
                :style="{ width: '50vw' }"
                :maximizable="modalConfig.maximizable"
                :closable="modalConfig.closable"
                :draggable="modalConfig.draggable"
                :center="true"
                :loading="modalConfig.loading"
                :header="modalConfig.header"
                :footer="modalConfig.footer"
              >
                <div class="border border-blue-200 rounded-lg p-6">
                  <div class="flex flex-col gap-4">
                    <template v-for="order in currentFunctionOrders" :key="order.orderID">
                      <div class="flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">工单ID：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold underline"
                          :value="order.orderID"
                          @click="handleOrderNumberClick(order.orderID)"
                          readonly
                        >
                      </div>
                      <!-- <div class="flex items-start gap-2">
                  <span class="w-32 text-black font-semibold">升级后模型版本</span>
                  <textarea
                    class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                    :value="order.finishModelVersionId"
                    rows="3"
                    readonly
                  />
                </div> -->
                      <div class="flex items-start gap-2">
                        <span class="w-32 text-black font-semibold">工单状态</span>
                        <input
                          class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                          :value="order.status"
                          readonly
                        >
                      </div>
                      <div class="flex items-start gap-2">
                        <span class="w-32 text-black font-semibold">基准版本</span>
                        <input
                          class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                          :value="order.baseModelVersion"
                          readonly
                        >
                      </div>
                      <div class="flex items-start gap-2">
                        <span class="w-32 text-black font-semibold">功能描述：</span>
                        <textarea
                          class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                          :value="order.finishFeatureDesc"
                          rows="3"
                          readonly
                        />
                      </div>
                      <hr class="my-2 border-gray-200">
                    </template>
                    <div v-if="currentFunctionOrders.length === 0" class="text-center text-gray-400">
                      暂无功能开发类工单
                    </div>
                  </div>
                </div>
                <template #footer>
                  <el-button type="primary" style="min-width: 100px;" @click="showFunctionDialog = false">
                    关闭
                  </el-button>
                </template>
              </FaModal>

              <!-- 复现状态弹窗 -->
              <FaModal
                v-model="showReproduceDialog"
                title="复现状态"
                :maximizable="modalConfig.maximizable"
                :closable="modalConfig.closable"
                :draggable="modalConfig.draggable"
                :center="true"
                :loading="modalConfig.loading"
                :header="modalConfig.header"
                :footer="modalConfig.footer"
              >
                <div class="border border-blue-200 rounded-lg p-6">
                  <div class="flex flex-col gap-4">
                    <template v-for="order in currentReproduceOrders" :key="order.orderID">
                      <div class="flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">工单ID：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold underline"
                          :value="order.orderID"
                          @click="handleOrderNumberClick(order.orderID)"
                          readonly
                        >
                      </div>
                      <div class="flex items-start gap-2">
                        <span class="w-32 text-black font-semibold">复现现象：</span>
                        <textarea
                          class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                          :value="order.finishPhenomenon"
                          rows="3"
                          readonly
                        />
                      </div>
                      <div class="flex items-start gap-2">
                        <span class="w-32 text-black font-semibold">备注：</span>
                        <textarea
                          class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                          :value="order.finishRemark"
                          rows="3"
                          readonly
                        />
                      </div>
                      <hr class="my-2 border-gray-200">
                    </template>
                    <div v-if="currentReproduceOrders.length === 0" class="text-center text-gray-400">
                      暂无问题复现类工单
                    </div>
                  </div>
                </div>
                <template #footer>
                  <el-button type="primary" style="min-width: 100px;" @click="showReproduceDialog = false">
                    关闭
                  </el-button>
                </template>
              </FaModal>

              <!-- 协调单信息弹窗 -->
              <FaModal
                v-model="showCoordinationDialog"
                title="协调单信息"
                :maximizable="modalConfig.maximizable"
                :closable="modalConfig.closable"
                :draggable="modalConfig.draggable"
                :center="true"
                :loading="modalConfig.loading"
                :header="modalConfig.header"
                :footer="modalConfig.footer"
              >
                <div class="border border-blue-200 rounded-lg p-6">
                  <div class="flex flex-col gap-4">
                    <template v-for="order in currentCoordinationOrders" :key="order.orderID">
                      <div class="flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">工单ID：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold underline"
                          :value="order.orderID"
                          @click="handleOrderNumberClick(order.orderID)"
                          readonly
                        >
                      </div>
                      <div class="flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">协调单ID：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold"
                          :value="order.coordinationID"
                          readonly
                        >
                      </div>
                      <hr class="my-2 border-gray-200">
                    </template>
                    <div v-if="currentCoordinationOrders.length === 0" class="text-center text-gray-400">
                      暂无协调单类工单
                    </div>
                  </div>
                </div>
                <template #footer>
                  <el-button type="primary" style="min-width: 100px;" @click="showCoordinationDialog = false">
                    关闭
                  </el-button>
                </template>
              </FaModal>

              <!-- 客户详细信息弹窗 -->
              <FaModal
                v-model="showClientDialog"
                title="客户详细信息"
                :maximizable="modalConfig.maximizable"
                :closable="modalConfig.closable"
                :draggable="modalConfig.draggable"
                :center="true"
                :loading="modalConfig.loading"
                :header="modalConfig.header"
                :footer="modalConfig.footer"
              >
                <div class="border border-blue-200 rounded-lg p-6">
                  <div class="flex flex-col gap-4">
                    <template v-for="order in currentDeliveryOrders" :key="order.orderID">
                      <div class="flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">工单ID：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold underline"
                          :value="order.orderID"
                          @click="handleOrderNumberClick(order.orderID)"
                          readonly
                        >
                      </div>
                      <div class="flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">目标客户：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold"
                          :value="order.targetCustomer"
                          readonly
                        >
                      </div>
                      <div class="flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">授权ID：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold"
                          :value="order.finishAuthId"
                          readonly
                        >
                      </div>
                      <div class="flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">外壳号：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold"
                          :value="order.finishShellNo"
                          readonly
                        >
                      </div>
                      <div class="flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">CAE-IPT平台验证状态：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                          :class="order.isCAEChecked ? 'text-green-700' : 'text-red-700'"
                          :value="order.isCAEChecked ? '已通过' : '未通过'"
                          readonly
                        >
                      </div>
                      <div class="flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">是否加密：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                          :class="order.isEncrypted ? 'text-green-700' : 'text-red-700'"
                          :value="order.isEncrypted ? '是' : '否'"
                          readonly
                        >
                      </div>
                      <hr class="my-2 border-gray-200">
                    </template>
                    <div v-if="currentDeliveryOrders.length === 0" class="text-center text-gray-400">
                      暂无交付发送类工单
                    </div>
                  </div>
                </div>
                <template #footer>
                  <el-button type="primary" style="min-width: 100px;" @click="showClientDialog = false">
                    关闭
                  </el-button>
                </template>
              </FaModal>

              <!-- 新增：模型版本弹窗 -->
              <el-dialog
                v-model="versionDialogVisible"
                title="模型版本批量操作"
                width="50vw"
                @open="fetchVersionTableData"
              >
                <el-table
                  v-loading="versionDialogLoading"
                  :data="versionTableData"
                  style="width: 100%;"
                  @selection-change="handleSelectionChange"
                >
                  <el-table-column type="selection" width="50" />
                  <el-table-column label="序号" type="index" width="60" />
                  <el-table-column prop="version" label="模型版本" width="180" />
                  <el-table-column prop="updateNotes" label="更新内容" />
                </el-table>
                <template #footer>
                  <el-button @click="versionDialogVisible = false">
                    取消
                  </el-button>
                  <el-button type="primary" :disabled="selectedVersions.length === 0" @click="submitSelectedVersions">
                    导出
                  </el-button>
                </template>
              </el-dialog>
              <div class="mt-6 flex justify-center">
                <el-pagination
                  background
                  layout="prev, pager, next"
                  :page-size="pageSize"
                  :current-page="page"
                  :total="total"
                  @current-change="handlePageChange"
                />
              </div>
            </template>
          </el-skeleton>
        </div>
      </template>
    </el-skeleton>
  </div>
</template>
