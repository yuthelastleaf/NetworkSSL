#pragma once

class AsyncTaskManager {
public:
    // ��ȡ����ʵ��
    static AsyncTaskManager& GetInstance() {
        static AsyncTaskManager instance; // C++11��֤��̬�ֲ������̰߳�ȫ
        return instance;
    }

    // ����첽����֧�����⺯���Ͳ�����
    template<typename Func, typename... Args>
    void AddTask(Func&& func, Args&&... args) {
        std::lock_guard<std::mutex> lock(mtx_);
        // �����첽���񲢱��� future
        futures_.emplace_back(
            std::async(std::launch::async,
                [func = std::forward<Func>(func)](auto&&... params) {
                    try {
                        func(std::forward<decltype(params)>(params)...);
                    }
                    catch (...) {
                        // ͳһ�쳣��������չΪ��־��¼��
                    }
                },
                std::forward<Args>(args)...
            )
        );
    }

    // �ȴ�����������ɣ���ѡ��
    void WaitAll() {
        std::lock_guard<std::mutex> lock(mtx_);
        for (auto& fut : futures_) {
            if (fut.valid()) fut.wait();
        }
    }

    // ��ֹ���ƺ��ƶ�
    AsyncTaskManager(const AsyncTaskManager&) = delete;
    AsyncTaskManager& operator=(const AsyncTaskManager&) = delete;

private:
    AsyncTaskManager() = default;
    ~AsyncTaskManager() {
        WaitAll(); // ����ʱȷ�������������
    }

    std::mutex mtx_;
    std::vector<std::future<void>> futures_;
};
