<script setup lang="ts">
import { ElMessage, ElMessageBox } from 'element-plus'
import { ref } from 'vue'

const isAdmin = true // 实际开发请根据权限判断
// --------------定义数据结构---------------
// 用户数据结构
interface User {
  jobNumber: string // 登录用户名（工号）
  fullName: string // 用户姓名（中文名）
  pasasword: string // 密码
  roleVec: string // 用户角色(“SuperUser”,'模型工程师','软件工程师','访客')
  canApprove: boolean // 是否为审批身份
  responsibleModel: string[] // 负责的模型ID列表
  email?: string // 邮箱
  phone?: string // 手机号
  createdTime?: string // 创建时间
  lastLoginTime?: string // 最后登录时间
  status: boolean // 在线状态，true=在线，false=离线
  lastModels?: string[] // 最近使用的模型
}

const userList = ref<User[]>([
  {
    jobNumber: '10001',
    fullName: 'admin',
    pasasword: '123456',
    roleVec: 'SuperUser',
    canApprove: true,
    responsibleModel: ['模型A', '模型B'],
    email: 'admin@example.com',
    phone: '13800000000',
    createdTime: '2024-01-01 10:00:00',
    lastLoginTime: '2024-07-14 09:00:00',
    status: true,
    lastModels: ['模型A'],
  },
  {
    jobNumber: '10002',
    fullName: '张三',
    pasasword: '654321',
    roleVec: '模型工程师',
    canApprove: false,
    responsibleModel: ['模型C'],
    email: 'zhangsan@example.com',
    phone: '13900000000',
    createdTime: '2024-02-01 11:00:00',
    lastLoginTime: '2024-07-13 08:00:00',
    status: false,
    lastModels: ['模型C'],
  },
])

// 空用户对象
const emptyUser: User = {
  jobNumber: '',
  fullName: '',
  pasasword: '',
  roleVec: '',
  canApprove: false,
  responsibleModel: [],
  email: '',
  phone: '',
  createdTime: '',
  lastLoginTime: '',
  status: true,
  lastModels: [],
}

const selectedUsers = ref<string[]>([])
const showEditDialog = ref(false)
const editUser = ref<User>({ ...emptyUser }) // 默认不是null

// 编辑用户
function handleEdit(user: User) {
  editUser.value = { ...user }
  showEditDialog.value = true
}

// 重置密码
function handleResetPwd(user: User) {
  ElMessageBox.confirm(`确定要重置用户 ${user.fullName} 的密码吗？`, '重置密码', {
    type: 'warning',
  }).then(() => {
    ElMessage.success('密码已重置')
  })
}

// 批量启用/禁用审批身份（带二次确认）
function handleBatchApprover(status: boolean) {
  ElMessageBox.confirm(
    `确定要批量${status ? '启用' : '禁用'}选中用户的审批身份吗？`,
    '审批身份批量变更',
    { type: 'warning' },
  ).then(() => {
    userList.value.forEach((user) => {
      if (selectedUsers.value.includes(user.jobNumber)) {
        user.canApprove = status
      }
    })
    ElMessage.success(`批量${status ? '启用' : '禁用'}审批身份成功`)
  })
}

// 保存用户信息
function handleSave() {
  if (!editUser.value) {
    return
  }
  // 判断是新建还是编辑
  const idx = userList.value.findIndex(u => u.jobNumber === editUser.value!.jobNumber)
  if (idx > -1) {
    // 编辑：角色变更二次确认
    const oldRole = userList.value[idx].roleVec
    const newRole = editUser.value.roleVec
    if (oldRole !== newRole) {
      ElMessageBox.confirm(
        `确定将用户 ${editUser.value.fullName} 的角色由 ${oldRole} 变更为 ${newRole}？`,
        '角色变更',
        { type: 'warning' },
      ).then(() => {
        userList.value[idx] = { ...editUser.value }
        ElMessage.success('保存成功')
        showEditDialog.value = false
      })
      return
    }
    // 编辑：更新原数据
    userList.value[idx] = { ...editUser.value }
  }
  else {
    // 新建：添加新用户
    userList.value.push({ ...editUser.value })
  }
  ElMessage.success('保存成功')
  showEditDialog.value = false
}

// 处理表格多选
function handleSelectionChange(selection: User[]) {
  selectedUsers.value = selection.map(u => u.jobNumber)
}
</script>

<template>
  <div v-if="isAdmin" class="h-full w-full flex flex-col p-6">
    <div class="mb-4 flex items-center justify-between">
      <span class="text-lg font-bold">用户管理</span>
      <el-button
        type="primary"
        @click="handleEdit({
          jobNumber: '',
          fullName: '',
          pasasword: '',
          roleVec: '',
          canApprove: false,
          responsibleModel: [],
          email: '',
          phone: '',
          createdTime: '',
          lastLoginTime: '',
          status: true,
          lastModels: [],
        })"
      >
        创建用户
      </el-button>
      <el-button
        type="success"
        :disabled="!selectedUsers.length"
        @click="handleBatchApprover(true)"
      >
        批量启用审批身份
      </el-button>
      <el-button
        type="danger"
        :disabled="!selectedUsers.length"
        @click="handleBatchApprover(false)"
      >
        批量禁用审批身份
      </el-button>
    </div>
    <div class="flex-1 overflow-auto">
      <el-table
        :data="userList"
        style="min-width: 800px;"
        border
        @row-click="handleEdit"
        @selection-change="handleSelectionChange"
      >
        <el-table-column type="selection" width="40" />
        <el-table-column prop="jobNumber" label="工号" width="90" />
        <el-table-column prop="fullName" label="用户名" width="80" />
        <el-table-column prop="roleVec" label="角色" width="120">
          <template #default="{ row }">
            <el-select v-model="row.roleVec" style="width: 90px;">
              <el-option label="SuperUser" value="SuperUser" />
              <el-option label="模型工程师" value="模型工程师" />
              <el-option label="软件工程师" value="软件工程师" />
              <el-option label="访客" value="访客" />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column prop="canApprove" label="审批身份" width="120">
          <template #default="{ row }">
            <el-switch v-model="row.canApprove" active-text="是" inactive-text="否" />
          </template>
        </el-table-column>
        <el-table-column prop="responsibleModel" label="负责模型" width="200">
          <template #default="{ row }">
            <el-select v-model="row.responsibleModel" filterable multiple placeholder="请选择模型" style="width: 160px;">
              <el-option label="模型A" value="模型A" />
              <el-option label="模型B" value="模型B" />
              <el-option label="模型C" value="模型C" />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column prop="status" label="在线状态" width="100">
          <template #default="{ row }">
            <el-switch
              v-model="row.status"
              active-text="在线"
              inactive-text="离线"
              :active-value="true"
              :inactive-value="false"
            />
          </template>
        </el-table-column>
        <el-table-column label="操作" width="200">
          <template #default="{ row }">
            <el-button type="primary" size="small" @click.stop="handleEdit(row)">
              编辑
            </el-button>
            <el-button type="warning" size="small" @click.stop="handleResetPwd(row)">
              重置密码
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </div>
    <!-- 用户编辑弹窗，包含所有字段 -->
    <el-dialog v-model="showEditDialog" title="用户编辑" width="600px">
      <el-form :model="editUser" label-width="100px">
        <el-form-item label="工号">
          <el-input v-model="editUser.jobNumber" placeholder="请输入工号" />
        </el-form-item>
        <el-form-item label="用户名">
          <el-input v-model="editUser.fullName" placeholder="请输入用户名" />
        </el-form-item>
        <el-form-item label="密码">
          <el-input v-model="editUser.pasasword" type="password" placeholder="请输入密码" />
        </el-form-item>
        <el-form-item label="角色">
          <el-select v-model="editUser.roleVec" style="width: 180px;">
            <el-option label="SuperUser" value="SuperUser" />
            <el-option label="模型工程师" value="模型工程师" />
            <el-option label="软件工程师" value="软件工程师" />
            <el-option label="访客" value="访客" />
          </el-select>
        </el-form-item>
        <el-form-item label="审批身份">
          <el-switch v-model="editUser.canApprove" active-text="是" inactive-text="否" />
        </el-form-item>
        <el-form-item label="负责模型">
          <el-select v-model="editUser.responsibleModel" multiple filterable placeholder="请选择模型" style="width: 220px;">
            <el-option label="模型A" value="模型A" />
            <el-option label="模型B" value="模型B" />
            <el-option label="模型C" value="模型C" />
          </el-select>
        </el-form-item>
        <el-form-item label="邮箱">
          <el-input v-model="editUser.email" placeholder="请输入邮箱" />
        </el-form-item>
        <el-form-item label="手机号">
          <el-input v-model="editUser.phone" placeholder="请输入手机号" />
        </el-form-item>
        <el-form-item label="创建时间">
          <el-input v-model="editUser.createdTime" placeholder="请输入创建时间" />
        </el-form-item>
        <el-form-item label="最后登录">
          <el-input v-model="editUser.lastLoginTime" placeholder="请输入最后登录时间" />
        </el-form-item>
        <el-form-item label="在线状态">
          <el-switch
            v-model="editUser.status"
            active-text="在线"
            inactive-text="离线"
            :active-value="true"
            :inactive-value="false"
          />
        </el-form-item>
        <el-form-item label="最近使用模型">
          <el-select v-model="editUser.lastModels" multiple filterable placeholder="请选择最近使用模型" style="width: 220px;">
            <el-option label="模型A" value="模型A" />
            <el-option label="模型B" value="模型B" />
            <el-option label="模型C" value="模型C" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showEditDialog = false">
          取消
        </el-button>
        <el-button type="primary" @click="handleSave">
          保存
        </el-button>
      </template>
    </el-dialog>
  </div>
  <div v-else class="p-6 text-center text-red-500">
    无权限访问
  </div>
</template>
