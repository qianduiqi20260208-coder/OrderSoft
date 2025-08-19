<route lang="yaml">
meta:
  title: 发送详情
</route>

<script setup lang="ts">
import { ElMessage } from 'element-plus'
import { onMounted, ref } from 'vue'
import { useRoute } from 'vue-router'
import deliveryApi from '@/api/modules/delivery'
// ----------------定义数据结构--------------------
// 添加路由实例
const route = useRoute() // 获取当前路由信息
const router = useRouter() // 添加路由实例
const clientName = ref('') // 客户名称
const modelCount = ref(0) // 发送模型数量
const modelVersionCount = ref(0) // 发送模型版本数量
const sendDetail = ref<SendDetail | null>(null) // 发送详情数据
const loading = ref(false) // 加载状态

// 分页相关变量
const pageSize = 10 // 每页显示10条日期记录
const currentPage = ref(1) // 当前页码
const total = ref(0) // 总日期数量

// 展开状态管理
const expandedMap = ref<Record<string, boolean>>({})

// 发送详情页面数据结构
interface SendDetail {
  clientName: string // 客户名称

  // 按日期分组的发送记录
  dailySendRecords: DailySendRecord[]
}

// 每日发送记录
interface DailySendRecord {
  date: string // 日期 (YYYY-MM-DD)
  sendCount: number // 当天发送数量
  records: SendRecord[] // 具体发送记录
}

// 具体发送记录
interface SendRecord {
  modelId: string // 模型ID（ATA04_Aerodynamics等）
  modelVersionId: string // 模型版本号
  orderId: string // 工单号
  sendTime: string // 发送时间 (YYYY-MM-DD HH:mm:ss)
}

// -------------事件处理函数---------------
// 格式化日期显示
function formatDate(dateStr: string): string {
  const today = new Date()
  const yesterday = new Date(today)
  yesterday.setDate(yesterday.getDate() - 1)

  if (dateStr === today.toISOString().split('T')[0]) {
    return `${dateStr} (今天)`
  }
  else if (dateStr === yesterday.toISOString().split('T')[0]) {
    return `${dateStr} (昨天)`
  }
  else {
    return dateStr
  }
}

// 控制每个日期记录的展开/收起
function expandDate(date: string, expand: boolean) {
  expandedMap.value[date] = expand
}

// ----------发送总览弹窗相关---------------
// 发送总览弹窗状态
const overviewDialogVisible = ref(false)
const overviewLoading = ref(false) // 总览加载状态
const overviewData = ref<SendRecord[]>([]) // 总览数据

// 发送总览处理函数
async function handleSendOverview() {
  overviewDialogVisible.value = true
  await fetchSendOverview()
}

// 获取发送总览数据
async function fetchSendOverview() {
  overviewLoading.value = true

  try {
    const res = await deliveryApi.getSendOverview(clientName.value)

    if (res?.data?.success) {
      // 手动数据映射处理，确保与前端接口一致
      const mappedOverviewData: SendRecord[] = (res.data.latestVersionRecords || []).map((record: any) => ({
        modelId: record.modelId || '', // 模型ID
        modelVersionId: record.modelVersionId || '', // 模型版本号
        orderId: record.orderId || '', // 工单号
        sendTime: record.sendTime || '', // 发送时间
      }))

      overviewData.value = mappedOverviewData
    }
    else {
      ElMessage.error(res?.data?.message || '获取发送总览失败')
      overviewData.value = []
    }
  }
  catch (error) {
    console.error('获取发送总览失败:', error)
    ElMessage.error('获取发送总览失败，请稍后重试')
    overviewData.value = []
  }
  finally {
    overviewLoading.value = false
  }
}

// 工单号跳转处理函数
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

// 获取发送详情数据（修改为支持分页）
async function fetchSendDetail(clientName: string, page = 1) {
  loading.value = true

  try {
    // 调用分页API接口
    const res = await deliveryApi.getSendDetailWithPagination({
      clientName,
      page,
      pageSize,
    })

    if (res?.data) {
      // 数据映射处理，确保与前端接口一致
      const mappedData: SendDetail = {
        clientName: res.data.clientName || clientName, // 客户名称
        // 按日期分组的发送记录（当前页的数据）
        dailySendRecords: (res.data.dailySendRecords || []).map((dailyRecord: any) => ({
          date: dailyRecord.date || '', // 日期
          sendCount: dailyRecord.sendCount || 0, // 当天发送数量
          // 具体发送记录
          records: (dailyRecord.records || []).map((record: any) => ({
            modelId: record.modelId || '', // 模型名称
            modelVersionId: record.modelVersionId || '', // 版本号
            orderId: record.orderId || '', // 工单号
            sendTime: record.sendTime || '', // 发送时间
          })),
        })),
      }

      sendDetail.value = mappedData

      // 更新分页信息
      total.value = res.data.total || 0 // 总日期数量
      currentPage.value = res.data.page || 1 // 当前页码

      // 初始化展开状态（默认都收起）
      expandedMap.value = {}
      mappedData.dailySendRecords.forEach((record) => {
        expandedMap.value[record.date] = false
      })

      console.warn('获取发送详情成功:', {
        page,
        total: total.value,
        dailyRecordsCount: mappedData.dailySendRecords.length,
      })
    }
    else {
      ElMessage.error('获取发送详情失败：响应数据为空')
    }
  }
  catch (error) {
    console.error('获取发送详情失败:', error)
    ElMessage.error('获取发送详情失败，请稍后重试')
  }
  finally {
    loading.value = false
  }
}

// 分页组件页码变化事件
function handlePageChange(page: number) {
  // 重置展开状态
  expandedMap.value = {}
  fetchSendDetail(clientName.value, page)
}

// 页面加载函数
onMounted(() => {
  clientName.value = route.query.clientName as string || ''
  modelCount.value = Number.parseInt(route.query.modelCount as string, 10) || 0
  modelVersionCount.value = Number.parseInt(route.query.modelVersionCount as string, 10) || 0

  if (clientName.value) {
    fetchSendDetail(clientName.value)
  }
})

// 返回按钮事件处理函数
function handleBack() {
  router.replace({ path: '/client_manage' })
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
              <h1 class="text-3xl text-gray-800 font-bold">
                {{ clientName }} - 发送详情
              </h1>
            </div>
            <div class="flex items-center gap-2">
              <FaButton
                class="border border-blue-500 rounded bg-blue-300 px-5 py-2 text-black font-semibold transition-colors duration-150 hover:bg-blue-400"
                @click="handleSendOverview"
              >
                发送总览
              </FaButton>
              <FaButton
                class="border border-gray-500 rounded bg-gray-300 px-5 py-2 text-black font-semibold transition-colors duration-150 hover:bg-gray-400"
                @click="handleBack"
              >
                返回客户管理
              </FaButton>
            </div>
          </div>

          <!-- 文字提示行和统计信息 -->
          <div class="mt-3">
            <!-- 统计概览 -->
            <div v-if="sendDetail" class="grid grid-cols-2 gap-6">
              <div class="flex items-center gap-3">
                <span class="text-sm text-gray-600">发送模型数量：</span>
                <span class="text-2xl text-blue-600 font-bold">{{ modelCount }}</span>
              </div>
              <div class="flex items-center gap-3">
                <span class="text-sm text-gray-600">发送模型版本数量：</span>
                <span class="text-2xl text-green-600 font-bold">{{ modelVersionCount }}</span>
              </div>
            </div>
          </div>
        </div>

        <!-- 每日发送记录 -->
        <div v-if="sendDetail">
          <FaPageMain
            v-for="dailyRecord in sendDetail.dailySendRecords"
            :key="dailyRecord.date"
            :title="dailyRecord.date"
            :collaspe="!expandedMap[dailyRecord.date]"
            height="auto"
            class="mb-4 w-full"
          >
            <template #title>
              <div class="w-full">
                <!-- 标题行 -->
                <div class="flex items-center gap-3">
                  <i class="i-mdi-calendar-today text-2xl text-black" />
                  <span class="text-xl text-blue-600 font-bold">
                    {{ formatDate(dailyRecord.date) }}
                  </span>
                </div>

                <!-- 展开/收起按钮和状态信息行 -->
                <div class="ml-2 mt-2 flex items-center gap-4">
                  <FaIcon
                    name="expand"
                    class="mr-2 cursor-pointer text-xl"
                    @click.stop="expandDate(dailyRecord.date, !expandedMap[dailyRecord.date])"
                  />
                  <span class="text-base text-black">
                    <span class="font-bold">发送数量：</span><span class="text-red-600 font-bold">{{ dailyRecord.sendCount }}</span>
                  </span>
                  <span class="text-base text-black">
                    <span class="font-bold">日期：</span>{{ dailyRecord.date }}
                  </span>
                </div>
              </div>
            </template>

            <!-- 当天发送记录列表 -->
            <div class="space-y-6">
              <div class="border border-green-200 rounded-lg p-6">
                <h3 class="mb-4 text-lg text-black font-bold">
                  发送记录
                </h3>
                <div class="space-y-4">
                  <div
                    v-for="record in dailyRecord.records"
                    :key="record.orderId"
                    class="border border-gray-200 rounded-lg bg-gray-50 p-4"
                  >
                    <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4">
                      <!-- 第一行：模型名称和工单号 -->
                      <div class="flex items-center gap-2">
                        <span class="w-24 text-black font-semibold">模型名称：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                          :value="record.modelId"
                          readonly
                        >
                      </div>
                      <div class="flex items-center gap-2">
                        <span class="w-24 text-black font-semibold">工单号：</span>
                        <div
                          class="flex-1 cursor-pointer border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm transition-colors duration-150 hover:bg-blue-50"
                          @click="handleOrderNumberClick(record.orderId)"
                        >
                          <span class="text-purple-700 font-bold underline hover:text-purple-900">
                            {{ record.orderId }}
                          </span>
                        </div>
                      </div>

                      <!-- 第二行：版本号 -->
                      <div class="flex items-center gap-2">
                        <span class="w-24 text-black font-semibold">版本号：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-green-700 font-bold"
                          :value="record.modelVersionId"
                          readonly
                        >
                      </div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </FaPageMain>
        </div>

        <!-- 空状态 -->
        <div v-else-if="!loading" class="border border-gray-200 rounded-lg bg-white p-6 text-center shadow-sm">
          <div class="text-gray-500">
            暂无发送记录
          </div>
        </div>

        <!-- 分页组件 -->
        <div v-if="total > 0" class="mt-6 flex justify-center">
          <el-pagination
            background
            layout="prev, pager, next"
            :page-size="pageSize"
            :current-page="currentPage"
            :total="total"
            @current-change="handlePageChange"
          />
        </div>

        <!-- 发送总览弹窗 -->
        <el-dialog
          v-model="overviewDialogVisible"
          title=""
          width="90%"
          :close-on-click-modal="false"
          style="max-width: 1200px;"
        >
          <template #header>
            <div class="w-full flex items-center justify-between">
              <span class="text-xl font-bold">发送总览 - 各模型最后发送版本</span>
            </div>
          </template>

          <div class="overflow-y-auto space-y-4" style="height: 580px; max-height: 80vh;">
            <!-- 加载状态 -->
            <el-skeleton v-if="overviewLoading" :rows="5" animated />

            <!-- 如果有最新版本记录 -->
            <div v-else-if="overviewData.length > 0">
              <div
                v-for="(record, index) in overviewData"
                :key="record.orderId"
              >
                <div class="border border-gray-200 rounded-lg bg-gray-50 p-4">
                  <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4">
                    <!-- 第一行：模型名称和工单号 -->
                    <div class="flex items-center gap-2">
                      <span class="w-28 text-black font-semibold">模型名称：</span>
                      <input
                        class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                        :value="record.modelId"
                        readonly
                      >
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-24 text-black font-semibold">工单号：</span>
                      <div
                        class="flex-1 cursor-pointer border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm transition-colors duration-150 hover:bg-blue-50"
                        @click="handleOrderNumberClick(record.orderId)"
                      >
                        <span class="text-purple-700 font-bold underline hover:text-purple-900">
                          {{ record.orderId }}
                        </span>
                      </div>
                    </div>

                    <!-- 第二行：版本号和更新时间 -->
                    <div class="flex items-center gap-2">
                      <span class="w-28 text-black font-semibold">最后发送版本：</span>
                      <input
                        class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-green-700 font-bold"
                        :value="record.modelVersionId"
                        readonly
                      >
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-24 text-black font-semibold">发送时间：</span>
                      <input
                        class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-gray-700 font-bold"
                        :value="record.sendTime"
                        readonly
                      >
                    </div>
                  </div>
                </div>

                <!-- 分割线，除了最后一条记录 -->
                <div
                  v-if="index < overviewData.length - 1"
                  class="my-4 border-t border-gray-300"
                />
              </div>
            </div>

            <!-- 空状态 -->
            <div v-else class="py-8 text-center text-gray-500">
              暂无模型发送记录
            </div>
          </div>

          <template #footer>
            <div class="flex justify-end gap-2">
              <FaButton
                class="border border-blue-500 rounded bg-blue-300 px-5 py-2 text-black font-semibold transition-colors duration-150 hover:bg-blue-400"
                :disabled="overviewLoading"
                @click="fetchSendOverview"
              >
                <el-icon v-if="overviewLoading" class="mr-1">
                  <Loading />
                </el-icon>
                刷新数据
              </FaButton>
              <FaButton
                class="border border-gray-500 rounded bg-gray-300 px-5 py-2 text-black font-semibold transition-colors duration-150 hover:bg-gray-400"
                @click="overviewDialogVisible = false"
              >
                关闭
              </FaButton>
            </div>
          </template>
        </el-dialog>
      </template>
    </el-skeleton>
  </div>
</template>
