<route lang="yaml">
meta:
  title: 测试页面
</route>

<script setup lang="ts">
import { ref } from 'vue'
import { toast } from 'vue-sonner'

const { auth } = useAuth()

const unit_name = ref('华模') // 分发单位
const shell_id = ref('A19000000001') // 外壳号
const model_version = ref('v1.0') // 模型版本
const reviewer1 = ref('John Doe') // 审核人1
const reviewer2 = ref('Jane Doe') // 审核人2
const auth_status = ref('authorized') // 授权状态
const auth_start_date = ref('2025-01-01') // 授权开始日期
const auth_end_date = ref('2025-12-31') // 授权结束日期
// const permanent_auth = ref(false) // 是否永久授权
const isEditing = ref(false)

function startEdit(permission: string | string[]) {
  if (auth(permission)) {
    isEditing.value = true
  }
  else {
    toast.error('无编辑权限', { position: 'top-center' })
  }
}
function saveEdit() {
  // 校验开始和结束日期格式
  if (!validateDate(auth_start_date.value) || !validateDate(auth_end_date.value)) {
    // 如果有一个日期格式不对，阻止保存
    return
  }
  isEditing.value = false
  // 可在此处添加保存逻辑
}
function validateDate(val: string) {
  // 正则：4位年-2位月-2位日
  const reg = /^\d{4}-\d{2}-\d{2}$/
  if (!reg.test(val)) {
    toast.error('日期格式应为 YYYY-MM-DD', { position: 'top-center' })
    return false
  }
  return true
}
</script>

<template>
  <div>
    <FaPageMain>
      你能看到这个页面，说明你有访问权限。
    </FaPageMain>
    <div>
      <FaPageMain>
        <div class="grid grid-cols-2 gap-4">
          <FaCard title="模型1" description="V1.2 20240223" class="w-120 flex flex-col justify-between">
            <div class="mb-4 flex flex-nowrap items-center gap-2">
              <span class="whitespace-nowrap">分发单位：</span>
              <template v-if="isEditing">
                <ElInput v-model="unit_name" placeholder="请输入内容" class="w-full" />
              </template>
              <template v-else>
                <span>{{ unit_name }}</span>
              </template>
            </div>
            <div class="mb-4 flex flex-nowrap items-center gap-2">
              <span class="whitespace-nowrap">外壳号：</span>
              <template v-if="isEditing">
                <ElInput v-model="shell_id" placeholder="请输入内容" class="w-full" />
              </template>
              <template v-else>
                <span>{{ shell_id }}</span>
              </template>
            </div>
            <div class="mb-4 flex flex-nowrap items-center gap-2">
              <span class="whitespace-nowrap">模型版本：</span>
              <template v-if="isEditing">
                <ElInput v-model="model_version" placeholder="请输入内容" class="w-full" />
              </template>
              <template v-else>
                <span>{{ model_version }}</span>
              </template>
            </div>
            <div class="mb-4 flex flex-nowrap items-center gap-2">
              <span class="whitespace-nowrap">审核人1: </span>
              <template v-if="isEditing">
                <ElInput v-model="reviewer1" placeholder="请输入内容" class="w-full" />
              </template>
              <template v-else>
                <span>{{ reviewer1 }}</span>
              </template>
            </div>
            <div class="mb-4 flex flex-nowrap items-center gap-2">
              <span class="whitespace-nowrap">审核人2: </span>
              <template v-if="isEditing">
                <ElInput v-model="reviewer2" placeholder="请输入内容" class="w-full" />
              </template>
              <template v-else>
                <span>{{ reviewer2 }}</span>
              </template>
            </div>
            <div class="mb-4 flex flex-nowrap items-center gap-2">
              <span class="whitespace-nowrap">授权状态：</span>
              <template v-if="isEditing">
                <ElInput v-model="auth_status" placeholder="请输入内容" class="w-full" />
                <!-- <FaDropdown
                  :items="[
                    [
                      { label: 'authorized' },
                      { label: 'pending' },
                      { label: 'denied' },
                    ],
                  ]"
                  @select="auth_status = $event"
                >
                  <FaButton>
                    <span>{{ auth_status }}</span>
                    <FaIcon name="i-ep:caret-bottom" />
                  </FaButton>
                </FaDropdown> -->
              </template>
              <template v-else>
                <span>{{ auth_status }}</span>
              </template>
            </div>
            <div class="mb-4 flex flex-nowrap items-center gap-2">
              <span class="whitespace-nowrap">授权开始日期：</span>
              <template v-if="isEditing">
                <ElInput v-model="auth_start_date" placeholder="请输入内容" class="w-full" />
              </template>
              <template v-else>
                <span>{{ auth_start_date }}</span>
              </template>
            </div>
            <div class="mb-4 flex flex-nowrap items-center gap-2">
              <span class="whitespace-nowrap">授权结束日期：</span>
              <template v-if="isEditing">
                <ElInput v-model="auth_end_date" placeholder="请输入内容" class="w-full" />
              </template>
              <template v-else>
                <span>{{ auth_end_date }}</span>
              </template>
            </div>
            <div class="flex justify-end">
              <ElButton v-if="!isEditing" type="primary" size="small" @click="startEdit(['permission.edit', 'permission.remove'])">
                编辑
              </ElButton>
              <ElButton v-else type="success" size="small" @click="saveEdit">
                保存
              </ElButton>
            </div>
          </FaCard>
          <FaCard title="卡片2" description="描述2" class="w-120">
            内容2
          </FaCard>
          <FaCard title="卡片3" description="描述3" class="w-120">
            内容3
          </FaCard>
          <FaCard title="卡片4" description="描述4" class="w-120">
            内容4
          </FaCard>
        </div>
      </FaPageMain>
    </div>
  </div>
</template>
