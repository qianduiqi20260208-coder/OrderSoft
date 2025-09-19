import { ElMessage } from 'element-plus'
import api from '@/api'

// 客户后缀数据类型
export interface ClientSuffix {
  clientName: string
  suffix: string
}

export const useClientSuffixStore = defineStore(
  'clientSuffix',
  () => {
    // 客户后缀列表
    const suffixList = ref<ClientSuffix[]>([])

    // 是否已加载数据
    const isLoaded = ref(false)

    // 获取所有客户后缀列表
    async function fetchClientSuffixes() {
      try {
        const response = await api.get('/client/suffixes')
        if (response.status === 1 && response.data?.suffixes) {
          suffixList.value = response.data.suffixes
          isLoaded.value = true
          console.log('客户后缀数据加载成功:', suffixList.value)
        }
        else {
          ElMessage.error('获取客户后缀失败')
        }
      }
      catch (error) {
        console.error('获取客户后缀接口调用失败:', error)
        ElMessage.error('获取客户后缀接口调用失败')
      }
    }

    // 根据客户名获取后缀列表
    function getSuffixesByClient(clientName: string): string[] {
      return suffixList.value
        .filter(item => item.clientName === clientName)
        .map(item => item.suffix)
    }

    // 验证后缀是否属于指定客户
    function validateSuffixForClient(clientName: string, suffix: string): boolean {
      const clientSuffixes = getSuffixesByClient(clientName)
      return clientSuffixes.includes(suffix)
    }

    // 获取所有客户名列表
    function getAllClientNames(): string[] {
      const clientNames = [...new Set(suffixList.value.map(item => item.clientName))]
      return clientNames
    }

    // 清空数据
    function clearSuffixData() {
      suffixList.value = []
      isLoaded.value = false
    }

    return {
      suffixList: readonly(suffixList),
      isLoaded: readonly(isLoaded),
      fetchClientSuffixes,
      getSuffixesByClient,
      validateSuffixForClient,
      getAllClientNames,
      clearSuffixData,
    }
  },
  {
   //@ts-ignore
    persist: {
      key: 'client-suffix-store',
      storage: sessionStorage,
      paths: ['suffixList', 'isLoaded'],
    },
  },
)
