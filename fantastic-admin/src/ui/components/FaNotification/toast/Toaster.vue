<script setup lang="ts">
import { isVNode } from 'vue'
import { Toast, ToastAction, ToastClose, ToastDescription, ToastProvider, ToastTitle, ToastViewport } from '.'
import { useToast } from './use-toast'

const { toasts } = useToast()
console.log("toasts,",toasts.value[0]);

</script>

<template>
  <ToastProvider>
    <Toast v-for="toast in toasts" :key="toast.id" v-bind="toast">
      <div class="flex gap-3">
        <!-- 图标区域 -->
        <div v-if="toast.icon" class="flex-shrink-0 mt-0.5">
          <div v-if="toast.icon === 'custom-todo'" class="w-5 h-5">
            <svg viewBox="0 0 1024 1024" class="w-full h-full text-blue-600 fill-current">
              <path d="M756.028 74c62.862 0 113.941 50.438 114.957 113.044l0.015 1.9v239.19l-2.436 74.715c-39.842-24.135-86.63-38.036-136.68-38.036-145.408 0-263.285 117.334-263.285 262.074 0 96.365 52.25 180.583 130.09 226.112l-123.7-0.82H171.972c-62.862 0-113.941-50.439-114.957-113.044L57 837.234v-648.29c0-62.847 50.45-113.914 113.07-114.929l1.902-0.015h584.056zM346.736 660.219H215.654c-16.505 0-29.886 13.38-29.886 29.886 0 16.505 13.38 29.885 29.886 29.885h131.082c16.505 0 29.885-13.38 29.885-29.885 0-16.506-13.38-29.886-29.885-29.886z m87.378-216.096h-218.46c-16.505 0-29.886 13.38-29.886 29.885 0 16.506 13.38 29.886 29.886 29.886h218.46c16.506 0 29.886-13.38 29.886-29.886 0-16.505-13.38-29.885-29.886-29.885z m212.32-216.097H215.661l-0.706 0.008c-16.183 0.375-29.187 13.608-29.187 29.878 0 16.505 13.384 29.885 29.893 29.885h430.773l0.706-0.008c16.183-0.374 29.187-13.608 29.187-29.877 0-16.506-13.383-29.886-29.893-29.886z" />
              <path d="M732 499c126.473 0 229 102.527 229 229S858.473 957 732 957 503 854.473 503 728s102.527-229 229-229z m24.166 103.568c-12.711 0-23.015 10.304-23.015 23.015l-0.001 109.674-65.91 78.004-0.431 0.525c-7.727 9.7-6.38 23.852 3.156 31.91 9.709 8.203 24.23 6.983 32.434-2.726l68.804-81.427c4.886-4.22 7.978-10.46 7.978-17.424v-0.284-118.252l-0.009-0.644c-0.341-12.413-10.51-22.371-23.006-22.371z" />
            </svg>
          </div>
          <FaIcon v-else :name="toast.icon" class="w-5 h-5" />
        </div>
        
        <!-- 内容区域 -->
        <div class="flex-1 grid gap-1">
          <ToastTitle v-if="toast.title">
            {{ toast.title }}
          </ToastTitle>
          <template v-if="toast.description">
            <ToastDescription v-if="isVNode(toast.description)">
              <component :is="toast.description" />
            </ToastDescription>
            <ToastDescription v-else>
              {{ toast.description }}
            </ToastDescription>
          </template>
        </div>
        
        <!-- 关闭按钮 -->
        <ToastClose />
      </div>
      <ToastAction v-if="toast.action && typeof toast.action === 'object' && 'altText' in toast.action" :alt-text="toast.action.altText" @click="toast.action.onClick">
        {{ toast.action.label }}
      </ToastAction>
    </Toast>
    <ToastViewport />
  </ToastProvider>
</template>
