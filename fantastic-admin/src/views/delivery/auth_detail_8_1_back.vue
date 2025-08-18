<route lang="yaml">
meta:
  title: 授权详情
</route>

<script setup lang="ts">
import { ElMessage } from 'element-plus'
import { onMounted, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import deliveryApi from '@/api/modules/delivery'

const route = useRoute() // 获取当前路由信息
const router = useRouter() // 获取路由实例
const clientName = ref('') // 客户名称
const authDetail = ref<AuthDetail | null>(null) // 授权详情数据
const loading = ref(false) // 加载状态

const expandedMap = ref<Record<string, boolean>>({}) // 展开状态管理

// 授权详情数据结构
interface AuthDetail {
  clientName: string // 客户名称
  dongleCount: number // 加密狗数量
  shellNumbers: ShellInfo[] // 外壳号信息列表
  totalAuthCount: number // 总授权ID数量
  validAuthCount: number // 有效授权ID数量
  expiringAuthCount: number // 临期授权ID数量
  expiredAuthCount: number // 过期授权ID数量
}

// 外壳号信息
interface ShellInfo {
  shellNumber: string // 外壳号
  deviceType: string // 客户设备类型 lab/IPT/FTD/FFS
  deviceNote: string // 客户设备备注
  authorizationList: AuthorizationInfo[] // 该外壳号下的授权信息列表
}

// 授权信息
interface AuthorizationInfo {
  authId: string // 授权ID
  startDate: string // 授权开始日期 (YYYY-MM-DD)
  endDate: string // 授权结束日期 (YYYY-MM-DD)
  authType: string // 授权类型 本地锁/网络锁/软锁授权
  authNote: string // 授权备注
  status: string // 授权状态（有效、临期、过期）
}

// 控制每个外壳号记录的展开/收起
function expandShell(shellNumber: string, expand: boolean) {
  expandedMap.value[shellNumber] = expand
}

// 计算剩余天数
function calculateRemainingDays(endDate: string): string {
  const today = new Date()
  const end = new Date(endDate)

  // 将时间重置为当天0点，避免时间差的影响
  today.setHours(0, 0, 0, 0)
  end.setHours(0, 0, 0, 0)

  const diffTime = end.getTime() - today.getTime()
  const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24))

  if (diffDays > 0) {
    return `${diffDays}天`
  }
  else if (diffDays === 0) {
    return '今天到期'
  }
  else {
    return `已过期${Math.abs(diffDays)}天`
  }
}

// 获取剩余天数颜色
function getRemainingDaysColor(endDate: string): string {
  const today = new Date()
  const end = new Date(endDate)

  // 将时间重置为当天0点，避免时间差的影响
  today.setHours(0, 0, 0, 0)
  end.setHours(0, 0, 0, 0)

  const diffTime = end.getTime() - today.getTime()
  const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24))

  if (diffDays > 5) {
    return 'text-green-600' // 有效：绿色
  }
  else if (diffDays >= 0 && diffDays <= 5) {
    return 'text-orange-600' // 临期：橙色
  }
  else {
    return 'text-gray-500' // 过期：灰色
  }
}

// 获取状态圆点颜色
function getStatusDotColor(status: string): string {
  switch (status) {
    case '有效': return 'bg-green-500'
    case '临期': return 'bg-orange-500'
    case '过期': return 'bg-gray-400'
    default: return 'bg-gray-400'
  }
}

// 获取外壳号状态圆点颜色
function getShellStatusDotColor(authorizationList: AuthorizationInfo[]): string {
  let hasValid = false
  let hasExpiring = false

  for (const auth of authorizationList) {
    const today = new Date()
    const end = new Date(auth.endDate)

    // 将时间重置为当天0点，避免时间差的影响
    today.setHours(0, 0, 0, 0)
    end.setHours(0, 0, 0, 0)

    const diffTime = end.getTime() - today.getTime()
    const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24))

    if (diffDays > 5) {
      hasValid = true
    }
    else if (diffDays >= 0 && diffDays <= 5) {
      hasExpiring = true
    }
  }

  // 优先级：有效 > 临期 > 过期
  if (hasValid) {
    return 'bg-green-500' // 至少有1个有效授权：绿色
  }
  else if (hasExpiring) {
    return 'bg-orange-500' // 至少有1个临期授权（但没有有效授权）：橙色
  }
  else {
    return 'bg-gray-400' // 所有授权都过期：灰色
  }
}

// 根据结束日期计算授权状态
function calculateAuthStatus(endDate: string): '有效' | '临期' | '过期' {
  const today = new Date()
  const end = new Date(endDate)

  // 将时间重置为当天0点，避免时间差的影响
  today.setHours(0, 0, 0, 0)
  end.setHours(0, 0, 0, 0)

  const diffTime = end.getTime() - today.getTime()
  const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24))

  if (diffDays > 5) {
    return '有效'
  }
  else if (diffDays >= 0 && diffDays <= 5) {
    return '临期'
  }
  else {
    return '过期'
  }
}

// 批量新建授权处理函数
function handleBatchCreateAuth() {
  console.warn('批量新建授权功能待实现')
  // TODO: 实现批量新建授权功能
  // 可能的实现方式：
  // 1. 打开批量新建授权对话框
  // 2. 跳转到批量新建授权页面
  // 3. 支持选择多个外壳号进行批量授权
  // router.push({ path: '/auth/batch-create', query: { clientName: clientName.value } })
}

// 授权模型处理函数
function handleAuthModel() {
  console.warn('授权模型功能待实现')
  // TODO: 实现授权模型跳转功能
  // 可能的实现方式：
  // router.push({ path: '/auth/model', query: { clientName: clientName.value } })
  // 或者跳转到其他相关页面：
  // router.push({ path: '/auth-model', query: { clientName: clientName.value } })
}

// ---------编辑外壳号相关信息----------------
// 编辑弹窗相关状态
const editDialogVisible = ref(false)
const editingShell = ref<ShellInfo | null>(null)
const editForm = ref({
  deviceType: '',
  deviceNote: '',
})

// 设备类型选项
const deviceTypeOptions = [
  { label: 'lab', value: 'lab' },
  { label: 'IPT', value: 'IPT' },
  { label: 'FTD', value: 'FTD' },
  { label: 'FFS', value: 'FFS' },
]

// 编辑外壳号信息处理函数
function handleEditShell(shellNumber: string) {
  const shell = authDetail.value?.shellNumbers.find(s => s.shellNumber === shellNumber)
  if (shell) {
    editingShell.value = shell
    editForm.value.deviceType = shell.deviceType
    editForm.value.deviceNote = shell.deviceNote
    editDialogVisible.value = true
  }
}

// 编辑外壳号信息提交
async function handleConfirmEdit() {
  if (!editingShell.value) {
    return
  }

  try {
    // 调用更新API
    const res = await deliveryApi.updateShellInfo({
      clientName: clientName.value, // 客户名称
      shellNumber: editingShell.value.shellNumber, // 外壳号
      deviceType: editForm.value.deviceType, // 设备类型
      deviceNote: editForm.value.deviceNote, // 设备备注
    })

    // 更新本地数据
    if (authDetail.value) {
      const shell = authDetail.value.shellNumbers.find(s => s.shellNumber === editingShell.value!.shellNumber)
      if (shell) {
        shell.deviceType = editForm.value.deviceType
        shell.deviceNote = editForm.value.deviceNote
      }
    }

    // 关闭弹窗
    editDialogVisible.value = false

    // 显示成功提示
    ElMessage.success(res.data.message || '外壳号信息更新成功')
  }
  catch (error) {
    console.error('更新外壳号信息失败:', error)
  }
}

// 取消编辑
function handleCancelEdit() {
  editDialogVisible.value = false
  editingShell.value = null
  editForm.value = {
    deviceType: '',
    deviceNote: '',
  }
}

// ------------新建授权弹窗---------------
// 新建授权弹窗相关状态
const createAuthDialogVisible = ref(false)
const createAuthShellNumber = ref('')
const createAuthForm = ref({
  authId: '',
  authType: '',
  startDate: '',
  endDate: '',
  authNote: '',
})

// 授权类型选项（根据数据库设计）
const authTypeOptions = [
  { label: '本地锁', value: '本地锁' },
  { label: '网络锁', value: '网络锁' },
  { label: '软锁授权', value: '软锁授权' },
]

// 为特定外壳号新建授权处理函数
function handleCreateAuthForShell(shellNumber: string) {
  createAuthShellNumber.value = shellNumber
  createAuthForm.value = {
    authId: '',
    authType: '',
    startDate: '',
    endDate: '',
    authNote: '',
  }
  createAuthDialogVisible.value = true
}

// 确认新建授权
async function handleConfirmCreateAuth() {
  try {
    // 表单验证
    if (!createAuthForm.value.authId.trim()) {
      console.warn('请填写授权ID')
      return
    }
    if (!createAuthForm.value.authType) {
      console.warn('请选择授权类型')
      return
    }
    if (!createAuthForm.value.startDate) {
      console.warn('请选择授权开始日期')
      return
    }
    if (!createAuthForm.value.endDate) {
      console.warn('请选择授权结束日期')
      return
    }
    if (!createAuthForm.value.authNote.trim()) {
      console.warn('请填写授权备注')
      return
    }

    // 日期验证
    if (new Date(createAuthForm.value.startDate) >= new Date(createAuthForm.value.endDate)) {
      console.warn('授权结束日期必须大于开始日期')
      return
    }

    // 调用新建授权API
    const res = await deliveryApi.createAuth({
      clientName: clientName.value,
      shellNumber: createAuthShellNumber.value,
      authId: createAuthForm.value.authId,
      authType: createAuthForm.value.authType,
      startDate: createAuthForm.value.startDate,
      endDate: createAuthForm.value.endDate,
      authNote: createAuthForm.value.authNote,
    })

    if (res?.data?.success) {
      // 关闭弹窗
      createAuthDialogVisible.value = false

      // 显示成功提示
      ElMessage.success(res.data.message || '新建授权成功')

      // 刷新页面数据
      if (clientName.value) {
        await fetchAuthDetail(clientName.value)
      }
    }
    else {
      ElMessage.error(res?.data?.message || '新建授权失败')
    }
  }
  catch (error) {
    console.error('新建授权失败:', error)
    ElMessage.error('新建授权失败')
  }
}

// 取消新建授权
function handleCancelCreateAuth() {
  createAuthDialogVisible.value = false
  createAuthShellNumber.value = ''
  createAuthForm.value = {
    authId: '',
    authType: '',
    startDate: '',
    endDate: '',
    authNote: '',
  }
}

// ------------批量更新授权弹窗---------------
// 批量更新授权弹窗相关状态
const batchUpdateDialogVisible = ref(false)
interface UpdateAuthItem {
  shellNumber: string
  authList: Array<{
    authId: string
    startDate: string
    endDate: string
    originalEndDate: string // 保存原始结束日期用于比较
  }>
}

const updateAuthList = ref<UpdateAuthItem[]>([])

// 批量更新授权处理函数
function handleBatchUpdateAuth() {
  if (!authDetail.value) {
    return
  }

  // 构建更新数据结构
  updateAuthList.value = authDetail.value.shellNumbers.map(shell => ({
    shellNumber: shell.shellNumber,
    authList: shell.authorizationList.map(auth => ({
      authId: auth.authId,
      startDate: auth.startDate,
      endDate: auth.endDate,
      originalEndDate: auth.endDate,
    })),
  }))

  batchUpdateDialogVisible.value = true
}

// 确认批量更新授权
async function handleConfirmBatchUpdate() {
  try {
    // 收集所有有变更的授权记录
    const changedAuths: Array<{
      shellNumber: string
      authId: string
      newEndDate: string
    }> = []

    updateAuthList.value.forEach((shell) => {
      shell.authList.forEach((auth) => {
        if (auth.endDate !== auth.originalEndDate) {
          changedAuths.push({
            shellNumber: shell.shellNumber,
            authId: auth.authId,
            newEndDate: auth.endDate,
          })
        }
      })
    })

    if (changedAuths.length === 0) {
      ElMessage.warning('没有检测到任何变更，无法进行批量更新')
      return
    }

    // 日期验证
    for (const changed of changedAuths) {
      const shell = updateAuthList.value.find(s => s.shellNumber === changed.shellNumber)
      const auth = shell?.authList.find(a => a.authId === changed.authId)
      if (auth && new Date(auth.startDate) >= new Date(auth.endDate)) {
        ElMessage.warning(`${changed.authId}: 授权结束日期必须大于开始日期`)
        return
      }
    }

    // 调用批量更新API
    const res = await deliveryApi.batchUpdateAuthEndDates({
      clientName: clientName.value,
      changes: changedAuths,
    })

    if (res?.data?.success) {
      // 更新本地数据
      if (authDetail.value) {
        updateAuthList.value.forEach((shellUpdate) => {
          const shell = authDetail.value!.shellNumbers.find(s => s.shellNumber === shellUpdate.shellNumber)
          if (shell) {
            shellUpdate.authList.forEach((authUpdate) => {
              const auth = shell.authorizationList.find(a => a.authId === authUpdate.authId)
              if (auth) {
                auth.endDate = authUpdate.endDate
                auth.status = calculateAuthStatus(authUpdate.endDate)
              }
            })
          }
        })
      }

      // 关闭弹窗
      batchUpdateDialogVisible.value = false

      // 显示成功提示
      ElMessage.success(res.data.message || '批量更新授权成功')

      // 刷新页面数据
      if (clientName.value) {
        await fetchAuthDetail(clientName.value)
      }
    }
  }
  catch (error) {
    ElMessage.error('批量更新授权失败')
    console.error('批量更新授权失败:', error)
  }
}

// 复位外壳号授权处理函数
function handleResetShellAuth(shellNumber: string) {
  const shell = updateAuthList.value.find(s => s.shellNumber === shellNumber)
  if (shell) {
    // 将该外壳号下的所有授权截止日期恢复为原始值
    shell.authList.forEach((auth) => {
      auth.endDate = auth.originalEndDate
    })

    console.warn(`已复位外壳号 ${shellNumber} 的所有授权修改`)
    ElMessage.success(`已复位外壳号 ${shellNumber} 的所有授权修改`)
  }
}

// 取消批量更新授权
function handleCancelBatchUpdate() {
  batchUpdateDialogVisible.value = false
  updateAuthList.value = []
}

// 获取批量更新时的剩余天数
function getBatchUpdateRemainingDays(endDate: string): string {
  return calculateRemainingDays(endDate)
}

// 获取批量更新时的剩余天数颜色
function getBatchUpdateRemainingDaysColor(endDate: string): string {
  return getRemainingDaysColor(endDate)
}

// --------------页面加载事件处理函数-----------------------
// 获取授权详情数据
async function fetchAuthDetail(clientName: string) {
  loading.value = true

  try {
    // 调用真实的API接口
    const res = await deliveryApi.getAuthDetail(clientName)

    if (res?.data) {
      // 数据映射处理，确保与前端接口一致
      const mappedData: AuthDetail = {
        clientName: res.data.clientName || clientName, // 客户名称
        dongleCount: res.data.dongleCount || 0, // 加密狗数量
        totalAuthCount: res.data.totalAuthCount || 0, // 总授权ID数量
        validAuthCount: res.data.validAuthCount || 0, // 有效授权ID数量
        expiringAuthCount: res.data.expiringAuthCount || 0, // 临期授权ID数量
        expiredAuthCount: res.data.expiredAuthCount || 0, // 过期授权ID数量
        shellNumbers: (res.data.shellNumbers || []).map((shell: any) => ({
          shellNumber: shell.shellNumber || '', // 外壳号
          deviceType: shell.deviceType || '', // 客户设备类型
          deviceNote: shell.deviceNote || '', // 客户设备备注
          authorizationList: (shell.authorizationList || []).map((auth: any) => ({
            authId: auth.authId || '', // 授权ID
            startDate: auth.startDate || '', // 授权开始日期
            endDate: auth.endDate || '', // 授权结束日期
            authType: auth.authType || '', // 授权类型
            authNote: auth.authNote || '', // 授权备注
            status: auth.status || calculateAuthStatus(auth.endDate || ''), // 授权状态
          })),
        })),
      }

      authDetail.value = mappedData // 更新授权详情数据

      // 初始化展开状态（默认都收起）
      expandedMap.value = {}
      mappedData.shellNumbers.forEach((shell) => {
        expandedMap.value[shell.shellNumber] = false
      })

      console.warn('获取授权详情成功')
    }
    else {
      ElMessage.error('获取授权详情失败：响应数据为空')
    }
  }
  catch (error) {
    console.error('获取授权详情失败:', error)
    ElMessage.error('获取授权详情失败，请稍后重试')
  }
  finally {
    loading.value = false
  }
}

// 页面加载时获取客户名称并请求授权详情
onMounted(() => {
  clientName.value = route.query.clientName as string || ''

  if (clientName.value) {
    fetchAuthDetail(clientName.value)
  }
})

// 返回客户管理页面
function handleBackToClientManage() {
  console.warn('返回客户管理页面')
  router.replace({ path: '/client_manage' })
}

// 返回加密狗管理页面
function handleBackToDongleManage() {
  console.warn('返回加密狗管理页面')
  router.replace({ path: '/dongle_manage' })
}
</script>

<template>
  <div>
    <el-skeleton :loading="loading" animated>
      <template #default>
        <!-- 顶部标题栏和统计信息 -->
        <div class="mb-6 border border-gray-200 rounded-lg bg-white p-6 shadow-sm">
          <!-- 第一行：标题和功能按钮 -->
          <div class="mb-4 flex items-center justify-between">
            <div class="flex items-center">
              <h1 class="text-3xl text-gray-800 font-bold">
                {{ clientName }} - 授权详情
              </h1>
            </div>
            <!-- 右边：返回按钮 -->
            <div class="flex items-center gap-3">
              <FaButton
                class="border border-gray-500 rounded bg-gray-300 px-4 py-2 text-sm text-black font-semibold transition-colors duration-150 hover:bg-gray-400"
                @click="handleBackToClientManage"
              >
                返回客户管理
              </FaButton>
              <FaButton
                class="border border-gray-500 rounded bg-gray-300 px-4 py-2 text-sm text-black font-semibold transition-colors duration-150 hover:bg-gray-400"
                @click="handleBackToDongleManage"
              >
                返回加密狗管理
              </FaButton>
            </div>
          </div>

          <!-- 第二行：统计信息和功能按钮 -->
          <div v-if="authDetail" class="flex items-center justify-between">
            <!-- 左边：统计信息 -->
            <div class="flex items-center gap-8">
              <div class="flex items-center gap-3">
                <span class="text-sm text-gray-600 font-bold">加密狗数量：</span>
                <span class="text-xl text-blue-600 font-bold">{{ authDetail.dongleCount }}</span>
              </div>
              <div class="flex items-center gap-3">
                <span class="text-sm text-gray-600 font-bold">授权ID（有效/临期/过期）：</span>
                <span class="text-xl font-bold">
                  <span class="text-green-600">{{ authDetail.validAuthCount }}</span>
                  <span class="text-gray-400">/</span>
                  <span class="text-yellow-600">{{ authDetail.expiringAuthCount }}</span>
                  <span class="text-gray-400">/</span>
                  <span class="text-gray-500">{{ authDetail.expiredAuthCount }}</span>
                </span>
              </div>
            </div>

            <!-- 右边：功能按钮 -->
            <div class="flex items-center gap-3">
              <FaButton
                class="border border-green-500 rounded bg-green-300 px-5 py-2 text-black font-semibold transition-colors duration-150 hover:bg-green-400"
                @click="handleBatchCreateAuth"
              >
                新建授权
              </FaButton>
              <FaButton
                class="border border-blue-500 rounded bg-blue-300 px-5 py-2 text-black font-semibold transition-colors duration-150 hover:bg-blue-400"
                @click="handleBatchUpdateAuth"
              >
                更新授权
              </FaButton>
              <FaButton
                class="border border-purple-500 rounded bg-purple-300 px-5 py-2 text-black font-semibold transition-colors duration-150 hover:bg-purple-400"
                @click="handleAuthModel"
              >
                授权模型
              </FaButton>
            </div>
          </div>
        </div>

        <!-- 外壳号列表 -->
        <div v-if="authDetail">
          <FaPageMain
            v-for="shell in authDetail.shellNumbers"
            :key="shell.shellNumber"
            :title="shell.shellNumber"
            height="auto"
            class="mb-4 w-full"
          >
            <template #title>
              <div class="w-full">
                <!-- 标题行 -->
                <div class="flex items-center justify-between">
                  <div class="flex items-center gap-3">
                    <!-- 外壳号状态圆点 -->
                    <div
                      class="h-4 w-4 flex-shrink-0 rounded-full"
                      :class="getShellStatusDotColor(shell.authorizationList)"
                    />
                    <span class="text-xl text-black font-bold">
                      {{ shell.shellNumber }}
                    </span>
                  </div>

                  <!-- 右上角按钮组 -->
                  <div class="mr-4 flex items-center gap-2">
                    <FaButton
                      class="border border-green-500 rounded bg-green-300 px-3 py-1 text-sm text-black font-semibold transition-colors duration-150 hover:bg-green-400"
                      @click.stop="handleCreateAuthForShell(shell.shellNumber)"
                    >
                      新建授权
                    </FaButton>
                    <FaButton
                      class="border border-blue-500 rounded bg-blue-300 px-3 py-1 text-sm text-black font-semibold transition-colors duration-150 hover:bg-blue-400"
                      @click.stop="handleEditShell(shell.shellNumber)"
                    >
                      编辑
                    </FaButton>
                  </div>
                </div>

                <!-- 展开/收起按钮和外壳号信息行 -->
                <div class="ml-2 mt-2 flex items-center gap-6">
                  <FaIcon
                    name="expand"
                    class="mr-2 cursor-pointer text-xl"
                    @click.stop="expandShell(shell.shellNumber, !expandedMap[shell.shellNumber])"
                  />
                  <span class="text-base text-black">
                    <span class="font-bold">设备类型：</span>{{ shell.deviceType }}
                  </span>
                  <span class="text-base text-black">
                    <span class="font-bold">设备备注：</span>{{ shell.deviceNote }}
                  </span>
                </div>
              </div>
            </template>

            <!-- 外壳号内容区域 -->
            <div class="space-y-4">
              <!-- 折叠状态下显示第一条授权信息 -->
              <div v-if="!expandedMap[shell.shellNumber] && shell.authorizationList.length > 0">
                <div class="border border-gray-200 rounded-lg bg-white shadow-sm">
                  <!-- 授权ID标题区域 -->
                  <div class="border-b border-gray-200 bg-gray-50 px-4 py-3">
                    <div class="flex items-center justify-between">
                      <div class="flex items-center gap-3">
                        <!-- 修改：增加gap间距 -->
                        <!-- 状态圆点 -->
                        <div
                          class="h-3 w-3 flex-shrink-0 rounded-full"
                          :class="getStatusDotColor(shell.authorizationList[0].status)"
                        />
                        <span class="text-lg text-black font-bold">{{ shell.authorizationList[0].authId }}</span>

                        <!-- 显示信息提示 - 移到标题右边 -->
                        <span class="flex items-center gap-1 text-base text-gray-500 font-medium">
                          <i class="i-mdi-information-outline text-blue-500" />
                          (当前显示第1条授权信息，共{{ shell.authorizationList.length }}条)
                        </span>
                      </div>
                      <!-- 右边留空或放其他内容 -->
                      <div />
                    </div>
                  </div>

                  <!-- 授权信息内容区域 -->
                  <div class="p-4 space-y-3">
                    <!-- 第一行：其他信息 -->
                    <div class="grid grid-cols-4 gap-4">
                      <div class="flex items-center gap-2">
                        <span class="w-20 text-sm text-gray-600">剩余天数：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm font-bold"
                          :class="getRemainingDaysColor(shell.authorizationList[0].endDate)"
                          :value="calculateRemainingDays(shell.authorizationList[0].endDate)"
                          readonly
                        >
                      </div>
                      <div class="flex items-center gap-2">
                        <span class="w-20 text-sm text-gray-600">开始日期：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-gray-700 font-bold"
                          :value="shell.authorizationList[0].startDate"
                          readonly
                        >
                      </div>
                      <div class="flex items-center gap-2">
                        <span class="w-20 text-sm text-gray-600">结束日期：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-gray-700 font-bold"
                          :value="shell.authorizationList[0].endDate"
                          readonly
                        >
                      </div>
                      <div class="flex items-center gap-2">
                        <span class="w-20 text-sm text-gray-600">授权类型：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-gray-700 font-bold"
                          :value="shell.authorizationList[0].authType"
                          readonly
                        >
                      </div>
                    </div>

                    <!-- 第二行：授权备注 -->
                    <div class="flex items-start gap-2">
                      <span class="mt-2 w-20 flex-shrink-0 text-sm text-gray-600">授权备注：</span>
                      <textarea
                        class="flex-1 resize-none overflow-y-auto border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-gray-700 font-bold"
                        :value="shell.authorizationList[0].authNote"
                        readonly
                        rows="2"
                      />
                    </div>
                  </div>
                </div>
              </div>

              <!-- 展开状态下显示所有授权信息 -->
              <div v-else-if="expandedMap[shell.shellNumber]">
                <div
                  v-for="auth in shell.authorizationList"
                  :key="auth.authId"
                  class="border border-gray-200 rounded-lg bg-white shadow-sm"
                >
                  <!-- 授权ID标题区域 -->
                  <div class="border-b border-gray-200 bg-gray-50 px-4 py-3">
                    <div class="flex items-center gap-2">
                      <!-- 状态圆点 -->
                      <div
                        class="h-3 w-3 flex-shrink-0 rounded-full"
                        :class="getStatusDotColor(auth.status)"
                      />
                      <span class="text-lg text-black font-bold">{{ auth.authId }}</span>
                    </div>
                  </div>

                  <!-- 授权信息内容区域 -->
                  <div class="p-4 space-y-3">
                    <!-- 展开状态下的第一行：其他信息 -->
                    <div class="grid grid-cols-4 gap-4">
                      <div class="flex items-center gap-2">
                        <span class="w-20 text-sm text-gray-600">剩余天数：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm font-bold"
                          :class="getRemainingDaysColor(auth.endDate)"
                          :value="calculateRemainingDays(auth.endDate)"
                          readonly
                        >
                      </div>
                      <div class="flex items-center gap-2">
                        <span class="w-20 text-sm text-gray-600">开始日期：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-gray-700 font-bold"
                          :value="auth.startDate"
                          readonly
                        >
                      </div>
                      <div class="flex items-center gap-2">
                        <span class="w-20 text-sm text-gray-600">结束日期：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-gray-700 font-bold"
                          :value="auth.endDate"
                          readonly
                        >
                      </div>
                      <div class="flex items-center gap-2">
                        <span class="w-20 text-sm text-gray-600">授权类型：</span>
                        <input
                          class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-gray-700 font-bold"
                          :value="auth.authType"
                          readonly
                        >
                      </div>
                    </div>

                    <!-- 第二行：授权备注 -->
                    <div class="flex items-start gap-2">
                      <span class="mt-2 w-20 flex-shrink-0 text-sm text-gray-600">授权备注：</span>
                      <textarea
                        class="flex-1 resize-none overflow-y-auto border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-gray-700 font-bold"
                        :value="auth.authNote"
                        readonly
                        rows="2"
                      />
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
            暂无授权信息
          </div>
        </div>

        <!-- 编辑外壳号信息弹窗 -->
        <el-dialog
          v-model="editDialogVisible"
          title="编辑外壳号信息"
          width="500px"
          :close-on-click-modal="false"
        >
          <template #header>
            <div class="w-full flex items-center justify-between">
              <span class="text-lg font-bold">编辑外壳号信息</span>
              <span class="text-base text-gray-600 font-bold">客户：{{ clientName }}</span>
            </div>
          </template>
          <div v-if="editingShell" class="space-y-4">
            <!-- 外壳号显示（只读） -->
            <div class="flex items-center gap-3">
              <label class="w-20 text-sm text-gray-600 font-bold">外壳号：</label>
              <el-input
                :value="editingShell.shellNumber"
                readonly
                class="flex-1"
              />
            </div>

            <!-- 设备类型选择 -->
            <div class="flex items-center gap-3">
              <label class="w-20 text-sm text-gray-600 font-bold">设备类型：</label>
              <el-select
                v-model="editForm.deviceType"
                placeholder="请选择设备类型"
                class="flex-1"
              >
                <el-option
                  v-for="option in deviceTypeOptions"
                  :key="option.value"
                  :label="option.label"
                  :value="option.value"
                />
              </el-select>
            </div>

            <!-- 设备备注输入 -->
            <div class="flex items-start gap-3">
              <label class="mt-2 w-20 flex-shrink-0 text-sm text-gray-600 font-bold">设备备注：</label>
              <el-input
                v-model="editForm.deviceNote"
                type="textarea"
                placeholder="请输入设备备注"
                :rows="3"
                maxlength="200"
                show-word-limit
                class="flex-1"
              />
            </div>
          </div>

          <template #footer>
            <div class="flex justify-end gap-3">
              <el-button @click="handleCancelEdit">
                取消
              </el-button>
              <el-button
                type="primary"
                :disabled="!editForm.deviceType.trim() || !editForm.deviceNote.trim()"
                @click="handleConfirmEdit"
              >
                确认
              </el-button>
            </div>
          </template>
        </el-dialog>

        <!-- 新建授权信息弹窗 -->
        <el-dialog
          v-model="createAuthDialogVisible"
          title="新建授权信息"
          width="600px"
          :close-on-click-modal="false"
        >
          <template #header>
            <div class="w-full flex items-center justify-between">
              <span class="text-lg font-bold">新建授权信息</span>
              <span class="text-base text-gray-600 font-bold">客户：{{ clientName }}</span>
            </div>
          </template>
          <div class="space-y-4">
            <!-- 外壳号显示（只读） -->
            <div class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">外壳号：</label>
              <el-input
                :value="createAuthShellNumber"
                readonly
                class="flex-1"
              />
            </div>

            <!-- 授权ID输入 -->
            <div class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">授权ID：</label>
              <el-input
                v-model="createAuthForm.authId"
                placeholder="请输入授权ID，十位纯数字"
                maxlength="50"
                class="flex-1"
              />
            </div>

            <!-- 授权类型选择 -->
            <div class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">授权类型：</label>
              <el-select
                v-model="createAuthForm.authType"
                placeholder="请选择授权类型"
                class="flex-1"
              >
                <el-option
                  v-for="option in authTypeOptions"
                  :key="option.value"
                  :label="option.label"
                  :value="option.value"
                />
              </el-select>
            </div>

            <!-- 授权开始日期 -->
            <div class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">开始日期：</label>
              <el-date-picker
                v-model="createAuthForm.startDate"
                type="date"
                placeholder="请选择授权开始日期"
                format="YYYY-MM-DD"
                value-format="YYYY-MM-DD"
                class="flex-1"
              />
            </div>

            <!-- 授权结束日期 -->
            <div class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">结束日期：</label>
              <el-date-picker
                v-model="createAuthForm.endDate"
                type="date"
                placeholder="请选择授权结束日期"
                format="YYYY-MM-DD"
                value-format="YYYY-MM-DD"
                :disabled-date="(date: Date) => createAuthForm.startDate ? date <= new Date(createAuthForm.startDate) : false"
                class="flex-1"
              />
            </div>

            <!-- 授权备注输入 -->
            <div class="flex items-start gap-3">
              <label class="mt-2 w-24 flex-shrink-0 text-sm text-gray-600 font-bold">授权备注：</label>
              <el-input
                v-model="createAuthForm.authNote"
                type="textarea"
                placeholder="请输入授权备注信息"
                :rows="4"
                maxlength="500"
                show-word-limit
                class="flex-1"
              />
            </div>
          </div>

          <template #footer>
            <div class="flex justify-end gap-3">
              <el-button @click="handleCancelCreateAuth">
                取消
              </el-button>
              <el-button
                type="primary"
                :disabled="!createAuthForm.authId.trim() || !createAuthForm.authType || !createAuthForm.startDate || !createAuthForm.endDate || !createAuthForm.authNote.trim()"
                @click="handleConfirmCreateAuth"
              >
                确认新建
              </el-button>
            </div>
          </template>
        </el-dialog>

        <!-- 批量更新授权弹窗 -->
        <el-dialog
          v-model="batchUpdateDialogVisible"
          title="批量更新授权"
          width="90%"
          :close-on-click-modal="false"
          class="max-h-[80vh]"
        >
          <template #header>
            <div class="w-full flex items-center justify-between">
              <span class="text-lg font-bold">批量更新授权</span>
              <span class="text-base text-gray-600 font-bold">客户：{{ clientName }}</span>
            </div>
          </template>
          <div class="max-h-[60vh] overflow-y-auto space-y-6">
            <!-- 提示信息 -->
            <div class="border border-blue-200 rounded bg-blue-50 p-3">
              <div class="text-sm text-blue-700">
                <i class="el-icon-info mr-2" />
                只能修改授权截止日期，修改后的日期必须大于授权开始日期。有变更的授权会以不同颜色标识。
              </div>
            </div>

            <!-- 外壳号列表 -->
            <div
              v-for="shell in updateAuthList"
              :key="shell.shellNumber"
              class="border border-gray-200 rounded-lg bg-white shadow-sm"
            >
              <!-- 外壳号标题 -->
              <div class="border-b border-gray-200 bg-gray-50 px-4 py-3">
                <div class="flex items-center justify-between">
                  <div class="flex items-center gap-3">
                    <i class="i-mdi-chip text-xl text-black" />
                    <span class="text-lg text-black font-bold">{{ shell.shellNumber }}</span>
                    <span class="text-sm text-gray-500">({{ shell.authList.length }}个授权)</span>
                  </div>

                  <!-- 复位按钮（右边） -->
                  <div class="flex items-center">
                    <FaButton
                      class="border border-gray-600 rounded bg-gray-300 px-2 py-1 text-xs text-black font-semibold transition-colors duration-150 hover:bg-gray-400"
                      @click="handleResetShellAuth(shell.shellNumber)"
                    >
                      复位
                    </FaButton>
                  </div>
                </div>
              </div>

              <!-- 授权列表 -->
              <div class="p-4 space-y-3">
                <div
                  v-for="auth in shell.authList"
                  :key="auth.authId"
                  class="border rounded-lg p-3 transition-colors duration-200"
                  :class="auth.endDate !== auth.originalEndDate ? 'border-blue-300 bg-blue-50' : 'border-gray-200 bg-gray-50'"
                >
                  <!-- 授权信息行 -->
                  <div class="grid grid-cols-5 items-center gap-3">
                    <!-- 授权ID -->
                    <div class="flex items-center gap-1">
                      <span class="whitespace-nowrap text-sm text-gray-600 font-bold">授权ID：</span>
                      <span class="truncate text-sm text-black font-bold">{{ auth.authId }}</span>
                    </div>

                    <!-- 开始日期 -->
                    <div class="flex items-center gap-1">
                      <span class="whitespace-nowrap text-sm text-gray-600 font-bold">开始日期：</span>
                      <span class="whitespace-nowrap text-sm text-gray-700">{{ auth.startDate }}</span>
                    </div>

                    <!-- 截止日期（可编辑） -->
                    <div class="flex items-center gap-1">
                      <span class="flex-shrink-0 whitespace-nowrap text-sm text-gray-600 font-bold">截止日期：</span>
                      <el-date-picker
                        v-model="auth.endDate"
                        type="date"
                        placeholder="选择截止日期"
                        format="YYYY-MM-DD"
                        value-format="YYYY-MM-DD"
                        :disabled-date="(date: Date) => date <= new Date(auth.startDate)"
                        size="small"
                        class="w-32 flex-shrink-0"
                      />
                    </div>

                    <!-- 剩余天数 -->
                    <div class="flex items-center gap-1">
                      <span class="flex-shrink-0 whitespace-nowrap text-sm text-gray-600 font-bold">剩余天数：</span>
                      <span
                        class="whitespace-nowrap text-sm font-bold"
                        :class="getBatchUpdateRemainingDaysColor(auth.endDate)"
                      >
                        {{ getBatchUpdateRemainingDays(auth.endDate) }}
                      </span>
                    </div>

                    <!-- 变更标识 -->
                    <div class="flex flex-shrink-0 justify-end">
                      <span
                        v-if="auth.endDate !== auth.originalEndDate"
                        class="whitespace-nowrap rounded bg-blue-100 px-2 py-1 text-xs text-blue-700 font-bold"
                      >
                        已修改
                      </span>
                      <span
                        v-else
                        class="whitespace-nowrap text-xs text-gray-400"
                      >
                        未修改
                      </span>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>

          <template #footer>
            <div class="flex items-center justify-between">
              <!-- 统计信息 -->
              <div class="text-sm text-gray-600">
                共 {{ updateAuthList.reduce((total, shell) => total + shell.authList.length, 0) }} 个授权，
                {{ updateAuthList.reduce((total, shell) => total + shell.authList.filter(auth => auth.endDate !== auth.originalEndDate).length, 0) }} 个有变更
              </div>

              <!-- 操作按钮 -->
              <div class="flex gap-3">
                <el-button @click="handleCancelBatchUpdate">
                  取消
                </el-button>
                <el-button
                  type="primary"
                  @click="handleConfirmBatchUpdate"
                >
                  确认更新
                </el-button>
              </div>
            </div>
          </template>
        </el-dialog>
      </template>
    </el-skeleton>
  </div>
</template>
