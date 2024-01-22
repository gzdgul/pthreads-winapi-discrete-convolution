#include <iostream>
#include <vector>
#include <pthread.h>
#include <chrono>
#include <random>
#include <iomanip>
#include <cstdlib>
#include <ctime>

// pthread_mutex_t mutex; // Mutex tanımlaması

struct ThreadData
{
    const std::vector<double> *x;
    const std::vector<double> *h;
    std::vector<double> *y;
    int start;
    int end;
};

void *threadFunction(void *args)
{
    ThreadData *data = (ThreadData *)args; // Tür dönüşümü
    const std::vector<double> &x = *(data->x);
    const std::vector<double> &h = *(data->h);
    std::vector<double> &y = *(data->y);
    int N = x.size(); // Giriş sinyali (x) vektörünün boyutu
    int M = h.size(); // Konvolüsyon çekirdeği (h) vektörünün boyutu

    for (int n = data->start; n <= data->end; n++) // İşaret edilen başlangıç noktasından bitiş noktasına kadar her bir eleman için
    {
        for (int k = 0; k < M; k++)
        {
            if (n - k >= 0 && n - k < N)
            {
                // pthread_mutex_lock(&mutex); // Mutex kilitle
                y[n] += x[n - k] * h[k];
                // pthread_mutex_unlock(&mutex); // Mutex kilidini aç
            }
        }
    }
    return nullptr;
}

std::vector<double> parallelConvolution(const std::vector<double> &x, const std::vector<double> &h, int thread_count)
{
    int N = x.size();                       // Giriş sinyali (x) vektörünün boyutu
    int M = h.size();                       // Konvolüsyon çekirdeği (h) vektörünün boyutu
    int outputSize = N + M - 1;             // Sonuç vektörünün boyutu
    std::vector<double> y(outputSize, 0.0); // y vektörü "outputSize" boyutunda ve her elemanı 0.0 dan başlar.

    std::vector<pthread_t> threads(thread_count);     // Belirlenen sayıda iş parçacığı oluştur // pthread_t türünde
    std::vector<ThreadData> threadData(thread_count); // Her iş parçacığı için veri paketlerini tutacak bir ThreadData nesnesi oluştur.

    int length = y.size() / thread_count;
    for (int i = 0; i < thread_count; i++)
    {
        threadData[i].x = &x;
        threadData[i].h = &h;
        threadData[i].y = &y;
        threadData[i].start = i * length;
        threadData[i].end = (i == thread_count - 1) ? y.size() - 1 : i * length + length - 1;

        int create_result = pthread_create(&threads[i], nullptr, threadFunction, &threadData[i]); // Her bir iş parçacığı oluşturulur
        if (create_result != 0)
        {
            std::cerr << "\033[31m(HATA)\033[0m Thread " << i << " oluşturulamadi. Hata kodu: " << create_result << " | " << std::endl;
        }
    }

    for (int i = 0; i < thread_count; i++)
    {
        int join_result = pthread_join(threads[i], nullptr);
        if (join_result != 0)
        {
            std::cerr << "\033[31m(HATA)\033[0m Thread " << i << " birleştirilemedi. Hata kodu: " << join_result << " | " << std::endl;
        }
    }
    return y;
}
std::vector<double> serialConvolution(const std::vector<double> &x, const std::vector<double> &h)
{
    int N = x.size();                       // Giriş sinyali (x) vektörünün boyutu
    int M = h.size();                       // Konvolüsyon çekirdeği (h) vektörünün boyutu
    int outputSize = N + M - 1;             // Sonuç vektörünün boyutu
    std::vector<double> y(outputSize, 0.0); // y vektörü "outputSize" boyutunda ve her elemanı 0.0 dan başlar.

    for (int n = 0; n < y.size(); n++) // y vektörünün her bir elemanı için
    {
        for (int k = 0; k < M; k++) // h vektörünün her bir elemanı için
        {
            if (n - k >= 0 && n - k < N) // n - k  0 dan küçük veya x'in boyutundan büyük olamaz.
            {
                y[n] += x[n - k] * h[k]; // x ve h vektörlerinin elemanlarının çarpımının y vektörüne toplanarak eklenir.
            }
        }
    }
    return y;
}
std::vector<double> generateRandomVector(size_t size)
{
    std::vector<double> randomVector(size);

    // Rastgele sayı üreticisini başlat
    std::srand(std::time(nullptr)); // rastgele sayı üreticisini başlatmak için çağrılır ve programın çalışma süresini temel alır.

    for (size_t i = 0; i < randomVector.size(); i++)
    {
        randomVector[i] = std::rand(); // Rastgele bir sayı üret
    }
    return randomVector;
}
int main()
{
    // Mutex başlatma
    // pthread_mutex_init(&mutex, nullptr);

    std::vector<int> N_values = {10, 100, 1000, 10000, 100000}; // Giriş sinyali (problem) boyutları
    int M = 10;                                                 // Konvolüsyon çekirdeği
    std::vector<int> T_values = {2, 4, 8, 16};                  // Thread sayıları

    // Tablo başlığı
    std::cout << " +---------------+----------+----------------+---------+--------------+" << std::endl;
    std::cout << " | Thread Sayisi | N Degeri | Sure (ms)      | Tip     |   Hizlanma   |" << std::endl;
    std::cout << " +---------------+----------+----------------+---------+--------------+" << std::endl;

    for (int N : N_values) // Her bir problem boyutu için
    {
        std::vector<double> x = generateRandomVector(N); // Giriş sinyali için N boyutlu rastgele elemanlardan oluşan bir vektör oluştur.
        std::vector<double> h = generateRandomVector(M); // Konvolüsyon çekirdeği için M boyutlu rastgele elemanlardan oluşan bir vektör oluştur.

        // Seri Konvolüsyon Testi
        auto start = std::chrono::high_resolution_clock::now();                 // İşlem başlangıç saati // auto dönüş türünü otomatik olarak algılar
        serialConvolution(x, h);                                                // Seri konvolüsyon işlemi
        auto end = std::chrono::high_resolution_clock::now();                   // İşlem bitiş saati
        std::chrono::duration<double, std::milli> elapsed_serial = end - start; // Seri işlem süresi (milisaniye cinsinden)

        // Tablo görünümü
        std::cout
            << " | "
            << "\033[33m" << std::setw(13) << 1
            << " | " << std::setw(8) << N
            << " | " << std::setw(14) << elapsed_serial.count()
            << " | "
            << "Seri   "
            << "\033[0m"
            << " | " << std::setw(12) << "-"
            << " | " << std::endl;

        // Tablo ara satır ayracı
        std::cout << " +---------------+----------+----------------+---------+--------------+" << std::endl;

        // Paralel Konvolüsyon Testleri
        for (int T : T_values) // Her bir thread sayısı için
        {
            auto start = std::chrono::high_resolution_clock::now();                   // İşlem başlangıç saati
            parallelConvolution(x, h, T);                                             // Paralel konvolüsyon işlemi
            auto end = std::chrono::high_resolution_clock::now();                     // İşlem bitiş saati
            std::chrono::duration<double, std::milli> elapsed_parallel = end - start; // Paralel işlem süresi (milisaniye cinsinden)

            // Tablo görünümü
            double speedup = elapsed_serial / elapsed_parallel;
            if (speedup > 1) // Hızlanma 1'den büyükse rengini yeşil yap
            {
                std::cout
                    << " | "
                    << "\033[32m" << std::setw(13) << T
                    << " | " << std::setw(8) << N
                    << " | " << std::setw(14) << elapsed_parallel.count()
                    << " | "
                    << "Paralel"
                    << " | " << std::setw(12) << speedup << "\033[0m"
                    << " | " << std::endl;
            }
            else // Hızlanma 1'den küçükse rengini kırmızı yap
            {
                std::cout
                    << " | " << std::setw(13) << T
                    << " | " << std::setw(8) << N
                    << " | " << std::setw(14) << elapsed_parallel.count()
                    << " | "
                    << "Paralel"
                    << " | "
                    << "\033[31m" << std::setw(12) << speedup << "\033[0m"
                    << " | " << std::endl;
            }
        }
        std::cout << " +---------------+----------+----------------+---------+--------------+" << std::endl;
    }
    // Mutex yok etme
    // pthread_mutex_destroy(&mutex);
    return 0;
}
