<script setup lang="ts">
import { toTypedSchema } from '@vee-validate/zod'
import { useForm } from 'vee-validate'
import * as z from 'zod'
import { FormControl, FormField, FormItem, FormMessage } from '@/ui/shadcn/ui/form'

defineOptions({
  name: 'ChangePasswordForm',
})

const props = defineProps<{
  account?: string
}>()

const emits = defineEmits<{
  onLogin: [account?: string]
  onChangePassword: [data: { account: string, oldPassword: string, newPassword: string }]
}>()

const loading = ref(false)

// 修改表单验证规则
const form = useForm({
  validationSchema: toTypedSchema(z.object({
    account: z.string().min(1, '请输入用户名'),
    oldPassword: z.string().min(1, '请输入原密码').min(3, '密码长度为3到18位').max(18, '密码长度为3到18位'),
    newPassword: z.string().min(1, '请输入新密码').min(3, '密码长度为3到18位').max(18, '密码长度为3到18位'),
    confirmPassword: z.string().min(1, '请确认新密码'),
  }).refine(data => data.newPassword === data.confirmPassword, {
    message: '两次输入的新密码不一致',
    path: ['confirmPassword'],
  }).refine(data => data.oldPassword !== data.newPassword, {
    message: '新密码不能与原密码相同',
    path: ['newPassword'],
  })),
  initialValues: {
    account: props.account ?? '',
    oldPassword: '',
    newPassword: '',
    confirmPassword: '',
  },
})

// 修改提交处理
const onSubmit = form.handleSubmit((values) => {
  loading.value = true

  // 发送修改密码数据
  emits('onChangePassword', {
    account: values.account,
    oldPassword: values.oldPassword,
    newPassword: values.newPassword,
  })
})
</script>

<template>
  <div class="min-h-500px w-full flex-col-stretch-center p-12">
    <form @submit="onSubmit">
      <div class="mb-8 space-y-2">
        <h3 class="text-4xl color-[var(--el-text-color-primary)] font-bold">
          修改密码
        </h3>
        <p class="text-sm text-muted-foreground">
          请输入原密码和新密码来修改您的账户密码
        </p>
      </div>

      <!-- 用户名 -->
      <FormField v-slot="{ componentField, errors }" name="account">
        <FormItem class="relative pb-6 space-y-0">
          <FormControl>
            <FaInput
              type="text"
              placeholder="请输入用户名"
              class="w-full"
              :class="errors.length && 'border-destructive'"
              v-bind="componentField"
            />
          </FormControl>
          <Transition enter-active-class="transition-opacity" enter-from-class="opacity-0" leave-active-class="transition-opacity" leave-to-class="opacity-0">
            <FormMessage class="absolute bottom-1 text-xs" />
          </Transition>
        </FormItem>
      </FormField>

      <!-- 原密码 -->
      <FormField v-slot="{ componentField, errors }" name="oldPassword">
        <FormItem class="relative pb-6 space-y-0">
          <FormControl>
            <FaInput
              type="password"
              placeholder="请输入原密码"
              class="w-full"
              :class="errors.length && 'border-destructive'"
              v-bind="componentField"
            />
          </FormControl>
          <Transition enter-active-class="transition-opacity" enter-from-class="opacity-0" leave-active-class="transition-opacity" leave-to-class="opacity-0">
            <FormMessage class="absolute bottom-1 text-xs" />
          </Transition>
        </FormItem>
      </FormField>

      <!-- 新密码 -->
      <FormField v-slot="{ componentField, errors }" name="newPassword">
        <FormItem class="relative pb-6 space-y-0">
          <FormControl>
            <FaInput
              type="password"
              placeholder="请输入新密码"
              class="w-full"
              :class="errors.length && 'border-destructive'"
              v-bind="componentField"
            />
          </FormControl>
          <Transition enter-active-class="transition-opacity" enter-from-class="opacity-0" leave-active-class="transition-opacity" leave-to-class="opacity-0">
            <FormMessage class="absolute bottom-1 text-xs" />
          </Transition>
        </FormItem>
      </FormField>

      <!-- 确认新密码 -->
      <FormField v-slot="{ componentField, errors }" name="confirmPassword">
        <FormItem class="relative pb-6 space-y-0">
          <FormControl>
            <FaInput
              type="password"
              placeholder="请再次输入新密码"
              class="w-full"
              :class="errors.length && 'border-destructive'"
              v-bind="componentField"
            />
          </FormControl>
          <Transition enter-active-class="transition-opacity" enter-from-class="opacity-0" leave-active-class="transition-opacity" leave-to-class="opacity-0">
            <FormMessage class="absolute bottom-1 text-xs" />
          </Transition>
        </FormItem>
      </FormField>

      <!-- 提交按钮 -->
      <FaButton :loading="loading" size="lg" class="mt-4 w-full" type="submit">
        确认修改
      </FaButton>

      <!-- 返回登录 -->
      <div class="mt-4 flex-center gap-2 text-sm color-[var(--el-text-color-secondary)]">
        <FaButton variant="link" class="h-auto p-0" @click="emits('onLogin', form.values.account)">
          返回登录
        </FaButton>
      </div>
    </form>
  </div>
</template>
