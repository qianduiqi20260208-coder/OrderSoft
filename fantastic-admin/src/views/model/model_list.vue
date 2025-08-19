<route lang="yaml">
meta:
  title:
  # enabled: false
</route>

<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { useRoute } from 'vue-router'
import modelApi from '@/api/modules/model'

// ----------------定义数据结构--------------------
const loading = ref(false) // loading变量定义
// 动态数据：模型列表，每个模型有多个工单
const modelList = ref<Model[]>([])
const route = useRoute() // 获取当前路由信息

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
  type: '问题复现' | '版本迭代' | '交付发送' | '版本迭代+交付发送' | '功能开发' | '其他'
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

// -------------后端事件处理函数---------------
// 预处理工单列表，将版本迭代+交付发送拆分为两个工单
function preprocessOrders(list: Model[]): Model[] {
  return list.map((model) => {
    const newOrders: Order[] = []
    model.orders.forEach((order) => {
      if (order.type === '版本迭代+交付发送') {
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
  const modelRouteName = route.matched[0].name // 获取当前页面对应的模型名称
  const params = {
    page: page.value,
    pageSize: pageSize.value,
    modelID: modelRouteName ? String(modelRouteName) : '', // 传递模型名称作为参数，确保为字符串
  } // 构造分页参数
  const res = await modelApi.getModelList(params) // 调用后端接口

  if (res?.data) {
    // 数据映射处理 - 将后端字段映射到前端期望的字段
    const mappedData = {
      list: (res.data.list || []).map((model: any) => ({
        modelID: model.modelID || '', // 模型id，就是模型名称
        version: model.version || '', // 模型版本ID
        updatetime: model.updatetime || '', // 模型此版本的更新时间
        orders: (model.orders || []).map((order: any) => ({
          // 通用字段
          orderID: order.orderID || '', // 工单ID
          type: order.type || '其他', // 工单类型
          startTime: order.startTime || '', // 工单发起时间
          promoterID: order.promoterID || '', // 工单发起人
          coordinationID: order.coordinationID || '', // 协调单ID
          baseModelVersion: order.baseModelVersion || '', // 模型父版本
          finishTime: order.finishTime || '', // 工单完成时间
          executorID: order.executorID || '', // 工单完成人ID

          // 完成工单相关字段
          finishRemark: order.finishRemark || '', // 工单完成备注

          // 问题复现类
          finishPhenomenon: order.finishPhenomenon || '', // 问题复现现象

          // 版本迭代类
          updateNotes: order.updateNotes || '', // updatenotes
          packingRequirement: order.packageRequirement || '', // 封装要求
          apiChanged: order.apiChanged || false, // 接口是否变化
          finishModelVersion: order.finishModelVersion || '', // 升级后模型版本ID（可选，主要用于版本迭代类工单）

          // 交付发送类
          isCAEChecked: order.isCAEChecked || false, // 是否通过CAE验证
          isEncrypted: order.isEncrypted || false, // 是否加密
          targetCustomer: order.targetCustomer || '', // 目标客户
          finishAuthId: order.finishAuthId || '', // 授权ID
          finishShellNo: order.finishShellNo || '', // 外壳号

          // 功能开发类
          finishFeatureDesc: order.finishFeatureDesc || '', // 功能描述
          finishModelVersionId: order.finishModelVersionId || '', // 升级后模型版本ID（可选，主要用于功能开发类工单）

          // 其他 - Mock数据中没有这个字段，保留兼容性
          finishRemarkOther: order.finishRemarkOther || order.description || '', // 其他类工单备注
        })),
      })),
      total: res.data.total || 0, // 模型版本数量
    }

    return mappedData
  }

  return { list: [], total: 0 } // 返回默认空数据
}

// 加载模型数据并处理 loading 状态
async function fetchUserModels() {
  loading.value = true
  try {
    const result = await apiFetchUserModels() // 获取数据
    // 预处理工单类型
    modelList.value = preprocessOrders(result.list || [])
    total.value = result.total || 0 // 设置总数
  }
  finally {
    loading.value = false // 关闭 loading
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
                  <span class="w-32 text-black font-semibold">接口变化：</span>
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
                <!-- 升级后模型版本 -->
                <div class="flex items-start gap-2">
                  <span class="w-32 text-black font-semibold">升级后模型版本</span>
                  <input
                    class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                    :value="order.finishModelVersion"
                    readonly
                  >
                </div>
                <!-- 模型父版本 -->
                <div class="flex items-start gap-2">
                  <span class="w-32 text-black font-semibold">模型父版本</span>
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
                    class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold"
                    :value="order.orderID"
                    readonly
                  >
                </div>
                <div class="flex items-start gap-2">
                  <span class="w-32 text-black font-semibold">升级后模型版本</span>
                  <textarea
                    class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                    :value="order.finishModelVersionId"
                    rows="3"
                    readonly
                  />
                </div>
                <div class="flex items-start gap-2">
                  <span class="w-32 text-black font-semibold">模型父版本</span>
                  <textarea
                    class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                    :value="order.baseModelVersion"
                    rows="3"
                    readonly
                  />
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
                    class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold"
                    :value="order.orderID"
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
                    class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold"
                    :value="order.orderID"
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
                    class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold"
                    :value="order.orderID"
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
                  <span class="w-32 text-black font-semibold">CAE平台验证状态：</span>
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
