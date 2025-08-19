<route lang="yaml">
meta:
  title: 客户管理
  # enabled: false
</route>

<script setup lang="ts">
import { ElMessage } from 'element-plus'
import { nextTick, onMounted, onUnmounted, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import deliveryApi from '@/api/modules/delivery'

// ----------------定义数据结构--------------------
// 添加路由实例
const route = useRoute()
const router = useRouter()
const loading = ref(false) // loading变量定义
// 动态数据：客户列表
const clientList = ref<Client[]>([])

// 定义客户信息管理数据结构
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

// -------------新建客户相关---------------
const createDialogVisible = ref(false)
const createFormRef = ref()
const createForm = ref({
  clientName: '',
  clientinfo: '',
})

// 新建客户表单验证规则
const createRules = {
  clientName: [
    { required: true, message: '请输入客户名称', trigger: 'blur' },
    { min: 2, max: 50, message: '客户名称长度在 2 到 50 个字符', trigger: 'blur' },
  ],
  clientinfo: [
    { max: 200, message: '客户信息备注不能超过 200 个字符', trigger: 'blur' },
  ],
}

// 新建客户
function handleCreateClient() {
  // 重置表单
  createForm.value = {
    clientName: '',
    clientinfo: '',
  }
  // 显示弹窗
  createDialogVisible.value = true
}

// 确认创建客户
async function handleConfirmCreate() {
  try {
    // 表单验证
    const valid = await createFormRef.value?.validate()
    if (!valid) {
      console.warn('表单验证失败')
      return
    }

    // 调用后端API创建客户
    const res = await deliveryApi.createClient({
      clientName: createForm.value.clientName,
      clientinfo: createForm.value.clientinfo,
    })

    if (res?.data?.success) {
      // 创建成功，前端自己构建新客户数据
      const newClient: Client = {
        clientName: createForm.value.clientName,
        dongleCount: 0, // 自动初始化为0
        modelCount: 0, // 自动初始化为0
        modelVersionCount: 0, // 自动初始化为0
        clientinfo: createForm.value.clientinfo,
        licenseStats: {
          validCount: 0, // 自动初始化为0
          expiringCount: 0, // 自动初始化为0
          expiredCount: 0, // 自动初始化为0
        },
      }

      // 添加到客户列表
      clientList.value.push(newClient)

      // 关闭弹窗
      createDialogVisible.value = false

      // 显示成功提示
      ElMessage.success(res.data.message || `客户 "${createForm.value.clientName}" 创建成功`)
    }
    else {
      // 创建失败，显示错误信息
      ElMessage.error(res?.data?.message || '创建客户失败')
    }
  }
  catch (error: any) {
    console.error('创建客户失败:', error)

    // 处理不同类型的错误
    if (error?.response?.data?.message) {
      ElMessage.error(error.response.data.message)
    }
    else if (error?.message) {
      ElMessage.error(error.message)
    }
    else {
      ElMessage.error('创建客户失败，请稍后重试')
    }
  }
}

// 取消创建客户
function handleCancelCreate() {
  // 重置表单
  createFormRef.value?.resetFields()
  // 关闭弹窗
  createDialogVisible.value = false
}

// -------------编辑客户相关---------------
const editDialogVisible = ref(false)
const editFormRef = ref()
const editForm = ref({
  clientName: '',
  clientinfo: '',
})
const originalClientName = ref('') // 保存原始客户名称

// 编辑客户表单验证规则
const editRules = {
  clientName: [
    { required: true, message: '请输入客户名称', trigger: 'blur' },
    { min: 2, max: 50, message: '客户名称长度在 2 到 50 个字符', trigger: 'blur' },
  ],
  clientinfo: [
    { max: 200, message: '客户信息备注不能超过 200 个字符', trigger: 'blur' },
  ],
}

// 编辑客户信息
function handleEdit(client: Client) {
  console.warn('编辑客户:', client.clientName)

  // 保存原始客户名称
  originalClientName.value = client.clientName

  // 填充表单数据
  editForm.value = {
    clientName: client.clientName,
    clientinfo: client.clientinfo || '',
  }

  // 显示弹窗
  editDialogVisible.value = true
}

// 确认编辑客户
async function handleConfirmEdit() {
  try {
    // 表单验证
    const valid = await editFormRef.value?.validate()
    if (!valid) {
      console.warn('表单验证失败')
      return
    }

    // 调用后端API编辑客户
    const res = await deliveryApi.updateClient({
      originalClientName: originalClientName.value, // 原始客户名称
      clientName: editForm.value.clientName, // 新客户名称
      clientinfo: editForm.value.clientinfo, // 新客户信息备注
    })

    if (res?.data?.success) {
      // 编辑成功 - 更新本地数据
      const clientIndex = clientList.value.findIndex(c => c.clientName === originalClientName.value)
      if (clientIndex !== -1) {
        // 更新客户信息
        clientList.value[clientIndex].clientName = editForm.value.clientName
        clientList.value[clientIndex].clientinfo = editForm.value.clientinfo
      }

      // 关闭弹窗
      editDialogVisible.value = false

      // 显示成功提示
      ElMessage.success(res.data.message || '客户信息修改成功')
    }
    else {
      // 编辑失败，显示错误信息
      ElMessage.error(res?.data?.message || '编辑客户失败')
    }
  }
  catch (error: any) {
    console.error('编辑客户失败:', error)

    // 处理不同类型的错误
    if (error?.response?.data?.message) {
      ElMessage.error(error.response.data.message)
    }
    else if (error?.message) {
      ElMessage.error(error.message)
    }
    else {
      ElMessage.error('编辑客户失败，请稍后重试')
    }
  }
}

// 取消编辑客户
function handleCancelEdit() {
  // 重置表单
  editFormRef.value?.resetFields()
  // 清空原始客户名称
  originalClientName.value = ''
  // 关闭弹窗
  editDialogVisible.value = false
}

// -------------后端事件处理函数---------------
// 计算授权总数
function getTotalLicenseCount(client: Client): number {
  return client.licenseStats.validCount + client.licenseStats.expiringCount + client.licenseStats.expiredCount
}

// 跳转发送详情
function handleSendDetail(client: Client) {
  router.push({
    path: '/client_manage/send_detail',
    query: {
      clientName: client.clientName, // 客户名称
      modelCount: client.modelCount, // 发送模型数量
      modelVersionCount: client.modelVersionCount, // 发送模型总版本数量
    },
  })
}

// 查看授权详情
function handleAuthDetail(client: Client) {
  // 跳转到授权详情页面，并传递客户名称参数
  router.push({
    path: '/client_manage/auth_detail',
    query: {
      clientName: client.clientName, // 客户名称
      dongleCount: client.dongleCount, // 加密狗数量
      validCount: client.licenseStats.validCount, // 有效授权数量
      expiringCount: client.licenseStats.expiringCount, // 临期授权数量（5天内过期）
      expiredCount: client.licenseStats.expiredCount, // 过期授权数量
    },
  })
}

// ----------------高亮功能相关函数--------------------
// 高亮相关状态
const highlightClientName = ref('')
const highlightTimeout = ref<NodeJS.Timeout | null>(null)
// 检查是否需要高亮某个客户
function checkHighlightClient() {
  const { highlightClient, fromDongleManage } = route.query

  if (highlightClient && fromDongleManage === 'true') {
    highlightClientName.value = highlightClient as string

    // 滚动到对应客户位置
    nextTick(() => {
      setTimeout(() => {
        scrollToClient(highlightClient as string)
      }, 100)
    })

    // 8秒后移除高亮效果
    highlightTimeout.value = setTimeout(() => {
      highlightClientName.value = ''
    }, 8000)
  }
}

// 滚动到指定客户位置
function scrollToClient(clientName: string) {
  const clientElement = document.querySelector(`[data-client-name="${clientName}"]`)
  if (clientElement) {
    clientElement.scrollIntoView({
      behavior: 'smooth',
      block: 'center',
    })
  }
}

// 检查客户是否需要高亮
function isClientHighlighted(clientName: string): boolean {
  return highlightClientName.value === clientName
}

// 获取高亮样式类
function getHighlightClass(clientName: string): string {
  if (isClientHighlighted(clientName)) {
    return 'highlight-client'
  }
  return ''
}

// 组件销毁时清理定时器
onUnmounted(() => {
  if (highlightTimeout.value) {
    clearTimeout(highlightTimeout.value)
  }
})

// 获取客户列表数据并处理
async function fetchClients() {
  loading.value = true
  try {
    // 直接调用API接口
    const res = await deliveryApi.getClientList()

    if (res?.data) {
      // 数据映射处理，确保与前端接口一致
      const mappedData: Client[] = (res.data.list || []).map((client: any) => ({
        clientName: client.clientName || '', // 客户名称
        dongleCount: client.dongleCount || 0, // 加密狗数量
        modelCount: client.modelCount || 0, // 发送模型数量
        modelVersionCount: client.modelVersionCount || 0, // 发送模型总版本数量
        licenseStats: {
          validCount: client.licenseStats?.validCount || 0, // 有效授权数量
          expiringCount: client.licenseStats?.expiringCount || 0, // 临期授权数量
          expiredCount: client.licenseStats?.expiredCount || 0, // 过期授权数量
        },
      }))

      clientList.value = mappedData
      console.warn('获取客户列表成功:', mappedData)
    }
    else {
      throw new Error('响应数据为空')
    }
  }
  catch (error) {
    console.error('获取客户列表失败:', error)
    ElMessage.error('获取客户列表失败，请稍后重试')
  }
  finally {
    loading.value = false
  }
}

// 页面初始化时加载数据
onMounted(() => {
  fetchClients()
  // 然后检查是否需要高亮
  checkHighlightClient()
})
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
                客户管理
              </h1>
            </div>
            <div class="flex items-center">
              <FaButton
                class="border border-green-500 rounded bg-green-300 px-5 py-2 text-black font-semibold transition-colors duration-150 hover:bg-green-400"
                @click="handleCreateClient"
              >
                新建客户
              </FaButton>
            </div>
          </div>

          <!-- 文字提示行 -->
          <div class="mt-3 text-sm text-gray-600">
            本页面为客户管理页面，提供客户信息的统计和管理功能
          </div>
        </div>

        <!-- 动态渲染所有客户 -->
        <FaPageMain
          v-for="client in clientList"
          :key="client.clientName"
          :title="client.clientName"
          :collaspe="false"
          height="auto"
          class="mb-4 w-full"
          :class="getHighlightClass(client.clientName)"
        >
          <template #title>
            <div class="w-full">
              <div class="flex items-center">
                <!-- 高亮指示器 - 只保留目标客户提示 -->
                <div
                  v-if="isClientHighlighted(client.clientName)"
                  class="mr-3 flex items-center"
                >
                  <div class="relative">
                    <!-- 主要指示器 -->
                    <div class="h-4 w-4 animate-pulse rounded-full bg-orange-500" />
                    <!-- 外圈动效 -->
                    <div class="absolute inset-0 h-4 w-4 animate-ping rounded-full bg-orange-400" />
                  </div>
                  <span class="ml-2 animate-bounce text-base text-orange-600 font-bold">
                    目标客户
                  </span>
                </div>

                <!-- 小绿点 -->
                <div class="mr-3 h-3 w-3 rounded-full bg-green-500" />
                <span class="text-2xl text-black font-bold">
                  {{ client.clientName }}
                </span>

                <!-- 右上角按钮区域 -->
                <div class="ml-auto mr-4 flex gap-2">
                  <FaButton
                    class="border border-blue-500 rounded bg-blue-300 px-5 py-1.5 text-black font-semibold transition-colors duration-150 hover:bg-blue-400"
                    @click="handleEdit(client)"
                  >
                    编辑
                  </FaButton>
                  <FaButton
                    class="border border-purple-500 rounded bg-purple-300 px-5 py-1.5 text-black font-semibold transition-colors duration-150 hover:bg-purple-400"
                    @click="handleSendDetail(client)"
                  >
                    发送详情
                  </FaButton>
                  <FaButton
                    class="border border-orange-500 rounded bg-orange-300 px-5 py-1.5 text-black font-semibold transition-colors duration-150 hover:bg-orange-400"
                    @click="handleAuthDetail(client)"
                  >
                    授权详情
                  </FaButton>
                </div>
              </div>
            </div>
          </template>

          <!-- 客户信息内容 -->
          <div class="px-4 py-2">
            <div class="flex flex-wrap items-center gap-x-8 gap-y-2">
              <span class="text-sm text-blue-700 font-bold">
                加密狗：<span class="text-base text-blue-600">{{ client.dongleCount }}</span>
              </span>
              <span class="text-sm text-green-700 font-bold">
                模型章节号：<span class="text-base text-green-600">{{ client.modelCount }}</span>
              </span>
              <span class="text-sm text-green-700 font-bold">
                子版本：<span class="text-base text-green-600">{{ client.modelVersionCount }}</span>
              </span>
            </div>
            <div class="mt-1 flex flex-wrap items-center gap-x-8 gap-y-2">
              <span class="text-sm text-gray-700 font-bold">
                有效授权：<span class="text-base text-green-500">{{ client.licenseStats.validCount }}</span>
              </span>
              <span class="text-sm text-yellow-700 font-bold">
                临期授权：<span class="text-base text-yellow-500">{{ client.licenseStats.expiringCount }}</span>
              </span>
              <span class="text-sm text-gray-700 font-bold">
                过期授权：<span class="text-base text-gray-400">{{ client.licenseStats.expiredCount }}</span>
              </span>
              <span class="text-sm text-black font-bold">
                总授权：<span class="text-base text-black">{{ getTotalLicenseCount(client) }}</span>
              </span>
              <span v-if="client.clientinfo" class="max-w-[300px] truncate text-sm text-gray-500 font-normal">
                备注：{{ client.clientinfo }}
              </span>
            </div>
          </div>
        </FaPageMain>
      </template>
    </el-skeleton>

    <!-- 新建客户弹窗 -->
    <el-dialog
      v-model="createDialogVisible"
      title="新建客户"
      width="50vw"
      :close-on-click-modal="false"
      :close-on-press-escape="false"
    >
      <el-form
        ref="createFormRef"
        :model="createForm"
        :rules="createRules"
        label-width="120px"
        label-position="left"
      >
        <el-form-item label="客户名称" prop="clientName">
          <el-input
            v-model="createForm.clientName"
            placeholder="请输入客户名称"

            maxlength="50"
            show-word-limit clearable
          />
        </el-form-item>

        <el-form-item label="客户信息备注" prop="clientinfo">
          <el-input
            v-model="createForm.clientinfo"
            type="textarea"
            placeholder="请输入客户信息备注（可选）"
            :rows="4"
            maxlength="200"
            show-word-limit
            resize="none"
          />
        </el-form-item>
      </el-form>

      <template #footer>
        <div class="flex justify-end gap-2">
          <el-button @click="handleCancelCreate">
            取消
          </el-button>
          <el-button type="primary" @click="handleConfirmCreate">
            确认创建
          </el-button>
        </div>
      </template>
    </el-dialog>

    <!-- 编辑客户弹窗 -->
    <el-dialog
      v-model="editDialogVisible"
      title="编辑客户"
      width="50vw"
      :close-on-click-modal="false"
      :close-on-press-escape="false"
    >
      <el-form
        ref="editFormRef"
        :model="editForm"
        :rules="editRules"
        label-width="120px"
        label-position="left"
      >
        <el-form-item label="客户名称" prop="clientName">
          <el-input
            v-model="editForm.clientName"
            placeholder="请输入客户名称"
            clearable
            maxlength="50"
            show-word-limit
          />
        </el-form-item>

        <el-form-item label="客户信息备注" prop="clientinfo">
          <el-input
            v-model="editForm.clientinfo"
            type="textarea"
            placeholder="请输入客户信息备注（可选）"
            :rows="4"
            maxlength="200"
            show-word-limit
            resize="none"
          />
        </el-form-item>
      </el-form>

      <template #footer>
        <div class="flex justify-end gap-2">
          <el-button @click="handleCancelEdit">
            取消
          </el-button>
          <el-button type="primary" @click="handleConfirmEdit">
            确认修改
          </el-button>
        </div>
      </template>
    </el-dialog>
  </div>
</template>

<style scoped>
/* 只保留指示器相关的动画效果 */

/* 跳动动画 */
.animate-bounce {
  animation: target-bounce 1.5s infinite;
}

@keyframes target-bounce {
  0%,
  100% {
    transform: translateY(0);
  }

  50% {
    transform: translateY(-6px);
  }
}

/* 脉冲动画 */
.animate-pulse {
  animation: target-pulse 2s cubic-bezier(0.4, 0, 0.6, 1) infinite;
}

@keyframes target-pulse {
  0%,
  100% {
    opacity: 1;
    transform: scale(1);
  }

  50% {
    opacity: 0.7;
    transform: scale(1.1);
  }
}

/* 外圈扩散动画 */
.animate-ping {
  animation: target-ping 1.5s cubic-bezier(0, 0, 0.2, 1) infinite;
}

@keyframes target-ping {
  0% {
    opacity: 0.8;
    transform: scale(1);
  }

  75%,
  100% {
    opacity: 0;
    transform: scale(2.5);
  }
}
</style>
