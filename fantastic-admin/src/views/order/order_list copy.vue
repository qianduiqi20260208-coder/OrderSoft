<route lang="yaml">
meta:
  title: 工单列表
  # enabled: false
</route>

<script setup lang="ts">
import { ref } from 'vue'

// -----------------数据结构-----------------
// 定义工单所包括的内容
interface Order {
  // 工单类型
  type: '问题复现' | '版本迭代' | '交付发送' | '版本迭代+交付发送' | '功能开发' | '其他' | ''
  // 工单状态
  status: '草稿' | '待审批' | '待分发' | '进行中' | '已完成' | '已退回' | ''
  // 参考优先级
  referencePriority?: '紧急' | '一般' | ''
  // 任务优先级
  taskPriority?: '紧急' | '一般' | ''

  orderID: string // 工单ID
  modelID: string // 模型ID
  modelName?: string // 模型名称
  modelVersionID: string // 模型版本ID
  promoterID: string // 发起人ID
  startTime: string // 发起时间
  approverID?: string // 审批人ID
  distributorID?: string // 分发人ID
  approveTime?: string // 审批时间
  distributeTime?: string // 分发时间
  executorID?: string // 执行人ID
  finishTime?: string // 完成时间

  // ----------问题复现工单（创建）----------
  coordinationID?: string // 对应协调单号
  description?: string // 复现内容
  referenceFile?: string // 复现参考文件

  // ----------版本迭代类（创建）----------
  parentVersionID?: string // 模型父版本ID
  updateNotes?: string // 更新内容
  packingRequirement?: string // 封装要求
  apiChange?: boolean // 接口是否变化

  // ----------交付发送类（创建）----------
  targetCustomer?: string // 目标客户
  isEncrypted?: boolean // 是否加密
  isCAEChecked?: boolean // 是否通过CAE平台验证

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
  finishAuthId?: string // 授权ID（数字）
  finishShellNo?: string // 外壳号（字母+数字）

  // 功能开发类（完成）
  finishModelID?: string // 完成后模型ID
  finishModelVersionId?: string // 完成后模型版本ID
  finishFeatureDesc?: string // 完成功能描述

  // 其他类（完成）
  finishRemarkOther?: string // 备注（完成）
}

// const orderList = ref<Order[]>([]) // 当前页面显示所需要的工单信息
const loading = ref(false) // loading变量定义，用于控制加载时显示的骨架屏
const pageSize = 10 // 每页显示的工单数量
const currentPage = ref(1) // 当前页码，默认为1
const total = ref(0) // 总工单数，默认为0

// // -----------------后端接口-----------------
// // 分页获取工单数据，默认显示第一页
// async function fetchOrderPage(page = 1) {
//   loading.value = true
//   try {
//     const res = await orderApi.fetchOrderPage({ page, pageSize })
//     orderList.value = res.data.list || []
//     total.value = res.data.total || 0
//     currentPage.value = res.data.page || 1
//   }
//   finally {
//     loading.value = false
//   }
// }

// function handlePageChange(page: number) {
//   fetchOrderPage(page) // 当分页组件页码变化时，重新获取对应页的数据
// }

// onMounted(() => {
//   fetchOrderPage(1) // 组件挂载时，加载第一页数据
// })

const orderList = ref<Order[]>([
  {
    orderID: '1001',
    type: '问题复现',
    status: '已完成',
    referencePriority: '紧急',
    taskPriority: '一般',
    modelID: 'MOD-001',
    modelVersionID: 'V1.0',
    promoterID: 'userA',
    startTime: '2025-07-15 09:00:00',
    coordinationID: 'COORD-001',
    description: '模型运行时出现异常，无法复现结果。',
    referenceFile: '',
    finishPhenomenon: '已定位问题，复现现象与描述一致。',
    finishRemark: '已修复，建议升级模型。',
    finishTime: '2025-07-15 12:00:00',
    approverID: 'userB',
    approveTime: '2025-07-15 09:30:00',
    distributorID: 'userC',
    distributeTime: '2025-07-15 10:00:00',
    executorID: 'userD',
  },
  {
    orderID: '1002',
    type: '版本迭代',
    status: '已完成',
    referencePriority: '一般',
    taskPriority: '紧急',
    modelID: 'MOD-002',
    modelVersionID: 'V2.0',
    promoterID: 'userE',
    startTime: '2025-07-14 08:00:00',
    parentVersionID: 'V1.5',
    updateNotes: '优化性能，增加新功能。',
    packingRequirement: '标准封装',
    apiChange: true,
    finishModelVersion: 'V2.0',
    finishRemark: '迭代完成，已上线。',
    finishTime: '2025-07-14 18:00:00',
    approverID: 'userF',
    approveTime: '2025-07-14 09:00:00',
    distributorID: 'userG',
    distributeTime: '2025-07-14 10:00:00',
    executorID: 'userH',
  },
  {
    orderID: '1003',
    type: '交付发送',
    status: '已完成',
    referencePriority: '一般',
    taskPriority: '一般',
    modelID: 'MOD-003',
    modelVersionID: 'V3.1',
    promoterID: 'userI',
    startTime: '2025-07-13 10:00:00',
    targetCustomer: '客户A',
    isEncrypted: true,
    isCAEChecked: false,
    finishAuthId: 'AUTH-20250713',
    finishShellNo: 'SHELL-003A',
    finishRemark: '交付完成，客户已验收。',
    finishTime: '2025-07-13 15:00:00',
    approverID: 'userJ',
    approveTime: '2025-07-13 11:00:00',
    distributorID: 'userK',
    distributeTime: '2025-07-13 12:00:00',
    executorID: 'userL',
  },
  {
    orderID: '1004',
    type: '版本迭代+交付发送',
    status: '已完成',
    referencePriority: '紧急',
    taskPriority: '紧急',
    modelID: 'MOD-004',
    modelVersionID: 'V4.0',
    promoterID: 'userM',
    startTime: '2025-07-12 09:00:00',
    parentVersionID: 'V3.5',
    updateNotes: '修复安全漏洞，提升稳定性。',
    packingRequirement: '高级封装',
    apiChange: false,
    targetCustomer: '客户B',
    isEncrypted: false,
    isCAEChecked: true,
    finishModelVersion: 'V4.0',
    finishAuthId: 'AUTH-20250712',
    finishShellNo: 'SHELL-004B',
    finishRemark: '升级与交付均已完成，客户反馈良好。',
    finishTime: '2025-07-12 17:00:00',
    approverID: 'userN',
    approveTime: '2025-07-12 10:00:00',
    distributorID: 'userO',
    distributeTime: '2025-07-12 11:00:00',
    executorID: 'userP',
  },
])
</script>

<template>
  <div>
    <el-skeleton :loading="loading" animated>
      <template #default>
        <div class="space-y-6">
          <FaPageMain
            v-for="order in orderList"
            :key="order.orderID"
            :title="`工单#${order.orderID}`"
            collaspe
            height="56px"
            class="mt-2"
          >
            <template #title>
              <div class="flex items-center gap-4">
                <span class="text-lg text-blue-900 font-bold">工单#{{ order.orderID }}</span>
                <span class="inline-block rounded bg-gray-100 px-3 py-1 text-base text-blue-800 font-semibold">
                  {{ order.type }}
                </span>
                <span
                  class="inline-block rounded px-3 py-1 text-sm font-semibold"
                  :class="{
                    'bg-green-100 text-green-700': order.status === '已完成',
                    'bg-blue-100 text-blue-700': order.status === '进行中',
                    'bg-yellow-100 text-yellow-700': order.status === '待审批' || order.status === '待分发',
                    'bg-gray-100 text-gray-700': order.status === '草稿',
                    'bg-red-100 text-red-700': order.status === '已退回',
                  }"
                >
                  {{ order.status }}
                </span>
              </div>
            </template>
            <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4">
              <!-- 工单基础信息和专有内容每行两个信息，不加外层蓝色边框 -->
              <div class="grid col-span-2 grid-cols-2 gap-x-8 gap-y-2">
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
                    <span class="w-32 text-black font-semibold">模型ID+版本：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="`${order.modelID} / ${order.modelVersionID}`" readonly>
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
                    <a v-if="order.referenceFile" :href="order.referenceFile" class="text-blue-600 underline" target="_blank">下载附件</a>
                    <span v-else class="text-gray-400">无</span>
                  </div>
                </template>

                <!-- 版本迭代类 -->
                <template v-else-if="order.type === '版本迭代'">
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">模型ID+版本：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="`${order.modelID} / ${order.modelVersionID}`" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">对应协调单：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.coordinationID || 'NA'" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">父版本ID：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.parentVersionID" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">更新内容：</span>
                    <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.updateNotes" rows="2" readonly />
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">封装要求：</span>
                    <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.packingRequirement" rows="2" readonly />
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">接口是否变化：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.apiChange ? '是' : '否'" readonly>
                  </div>
                </template>

                <!-- 交付发送类 -->
                <template v-else-if="order.type === '交付发送'">
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">模型ID+版本：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="`${order.modelID} / ${order.modelVersionID}`" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">目标客户：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.targetCustomer" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">是否加密：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.isEncrypted ? '是' : '否'" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">CAE平台验证：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.isCAEChecked ? '是' : '否'" readonly>
                  </div>
                </template>

                <!-- 版本迭代+交付发送类工单（创建时相关信息显示） -->
                <template v-if="order.type === '版本迭代+交付发送'">
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">模型ID+版本：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="`${order.modelID} / ${order.modelVersionID}`" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">对应协调单：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.coordinationID || 'NA'" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">父版本ID：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.parentVersionID" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">更新内容：</span>
                    <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.updateNotes" rows="2" readonly />
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">封装要求：</span>
                    <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.packingRequirement" rows="2" readonly />
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">接口是否变化：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.apiChange ? '是' : '否'" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">目标客户：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.targetCustomer" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">是否加密：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.isEncrypted ? '是' : '否'" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">CAE平台验证：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.isCAEChecked ? '是' : '否'" readonly>
                  </div>
                </template>

                <!-- 功能开发类 -->
                <template v-else-if="order.type === '功能开发'">
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">模型ID+版本：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="`${order.modelID} / ${order.modelVersionID}`" readonly>
                  </div>
                  <div class="flex items-start gap-2">
                    <span class="w-32 text-black font-semibold">功能描述：</span>
                    <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.featureDesc" rows="2" readonly />
                  </div>
                </template>

                <!-- 其他类 -->
                <template v-else>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">模型ID+版本：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="`${order.modelID} / ${order.modelVersionID}`" readonly>
                  </div>
                  <div class="flex items-start gap-2">
                    <span class="w-32 text-black font-semibold">内容描述：</span>
                    <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.contentDesc" rows="2" readonly />
                  </div>
                </template>
              </div>

              <hr class="col-span-2 my-4 border-t border-gray-300">

              <!-- 通用信息2 -->
              <div class="col-span-1 w-full flex items-center gap-2">
                <span class="w-32 text-black font-semibold">审批人：</span>
                <input
                  class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                  :class="['待分发', '进行中', '已完成', '已退回'].includes(order.status) && order.approverID ? 'text-blue-700' : 'text-gray-400'"
                  :value="['待分发', '进行中', '已完成', '已退回'].includes(order.status) ? (order.approverID || '无') : '未到此环节'"
                  readonly
                >
              </div>
              <div class="col-span-1 w-full flex items-center gap-2">
                <span class="w-32 text-black font-semibold">审批时间：</span>
                <input
                  class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                  :class="['待分发', '进行中', '已完成', '已退回'].includes(order.status) && order.approveTime ? 'text-blue-700' : 'text-gray-400'"
                  :value="['待分发', '进行中', '已完成', '已退回'].includes(order.status) ? (order.approveTime || '无') : '未到此环节'"
                  readonly
                >
              </div>
              <div class="col-span-1 w-full flex items-center gap-2">
                <span class="w-32 text-black font-semibold">分发人：</span>
                <input
                  class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                  :class="['进行中', '已完成', '已退回'].includes(order.status) && order.distributorID ? 'text-blue-700' : 'text-gray-400'"
                  :value="['进行中', '已完成', '已退回'].includes(order.status) ? (order.distributorID || '无') : '未到此环节'"
                  readonly
                >
              </div>
              <div class="col-span-1 w-full flex items-center gap-2">
                <span class="w-32 text-black font-semibold">分发时间：</span>
                <input
                  class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                  :class="['进行中', '已完成', '已退回'].includes(order.status) && order.distributeTime ? 'text-blue-700' : 'text-gray-400'"
                  :value="['进行中', '已完成', '已退回'].includes(order.status) ? (order.distributeTime || '无') : '未到此环节'"
                  readonly
                >
              </div>
              <div class="col-span-1 w-full flex items-center gap-2">
                <span class="w-32 text-black font-semibold">执行人ID：</span>
                <input
                  class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                  :class="['进行中', '已完成', '已退回'].includes(order.status) && order.executorID ? 'text-blue-700' : 'text-gray-400'"
                  :value="['进行中', '已完成', '已退回'].includes(order.status) ? (order.executorID || '无') : '未到此环节'"
                  readonly
                >
              </div>
              <div class="col-span-1 w-full flex items-center gap-2">
                <span class="w-32 text-black font-semibold">工单完成类型：</span>
                <input
                  class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                  :class="order.status === '已完成' && order.type ? 'text-blue-700' : 'text-gray-400'"
                  :value="order.status === '已完成' ? (order.type || '无') : '未到此环节'"
                  readonly
                >
              </div>

              <!-- 完成类型细分：仅已完成时显示 -->
              <template v-if="order.status === '已完成'">
                <template v-if="order.type === '问题复现'">
                  <!-- 问题复现类工单 -->
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">复现现象：</span>
                    <textarea
                      class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="order.finishPhenomenon" rows="2" readonly
                    />
                  </div>
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">备注：</span>
                    <textarea
                      class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="order.finishRemark" rows="2" readonly
                    />
                  </div>
                </template>
                <!-- 版本迭代类工单 -->
                <template v-else-if="order.type === '版本迭代'">
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">升级后模型版本：</span>
                    <input
                      class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="order.finishModelVersion" readonly
                    >
                  </div>
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">备注：</span>
                    <textarea
                      class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="order.finishRemark" rows="2" readonly
                    />
                  </div>
                </template>
                <!-- 交付发送类工单 -->
                <template v-else-if="order.type === '交付发送'">
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">授权ID：</span>
                    <input
                      class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="order.finishAuthId" readonly
                    >
                  </div>
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">外壳号：</span>
                    <input
                      class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="order.finishShellNo" readonly
                    >
                  </div>
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">备注：</span>
                    <textarea
                      class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="order.finishRemark" rows="2" readonly
                    />
                  </div>
                </template>
                <!-- 版本迭代+交付发送类工单 -->
                <template v-if="order.type === '版本迭代+交付发送'">
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">升级后模型版本：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.finishModelVersion" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">授权ID：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.finishAuthId" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">外壳号：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.finishShellNo" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">备注：</span>
                    <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.finishRemark" rows="2" readonly />
                  </div>
                </template>
                <!-- 功能开发类工单 -->
                <template v-else-if="order.type === '功能开发'">
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">完成后模型ID：</span>
                    <input
                      class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="order.finishModelID" readonly
                    >
                  </div>
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">完成后模型版本ID：</span>
                    <input
                      class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="order.finishModelVersionId" readonly
                    >
                  </div>
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">完成功能描述：</span>
                    <textarea
                      class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="order.finishFeatureDesc" rows="2" readonly
                    />
                  </div>
                </template>
                <template v-else>
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">备注：</span>
                    <textarea
                      class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                      :value="order.finishRemark" rows="2" readonly
                    />
                  </div>
                </template>
              </template>

              <div class="col-span-1 w-full flex items-center gap-2">
                <span class="w-32 text-black font-semibold">完成时间：</span>
                <input
                  class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 font-bold"
                  :class="order.status === '已完成' && order.finishTime ? 'text-blue-700' : 'text-gray-400'"
                  :value="order.status === '已完成' ? (order.finishTime || '无') : '未到此环节'"
                  readonly
                >
              </div>
              <div class="mt-6" />
            </div>
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
      </template>
    </el-skeleton>
  </div>
</template>
