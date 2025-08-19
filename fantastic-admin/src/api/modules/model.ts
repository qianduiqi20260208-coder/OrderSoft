import api from '../index'

export default {
  // 获取模型列表及工单信息
  getModelList: (params: {
    page: number // 当前页码
    pageSize: number // 每页显示数量
    modelID: string // 模型ID
  }) =>
    api.get('model/list', {
      // baseURL: '/mock/', // mock数据接口地址
      params, // 请求参数
    }),
}
