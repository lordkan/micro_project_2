#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <mutex>
#include <sstream>
#include <thread>
#include <random>
#include <condition_variable>

using namespace std;

// Конец работы докторов.
bool finish = false;
// События для синхронизации потоков.
bool bserverready = false, bexchange1 = false, bexchange2 = false, bserveranswer = false;
//
condition_variable cserverready, cexchange1, cexchange2, cserveranswer;
// Мутексы для синхронизации потоков.
mutex serverready, exchange1, exchange2, serveranswer;
// Мутекс для безопасного вывода.
mutex m_write;

// Безопасный вывод при многопоточности.
void write(stringstream &message) {
    m_write.lock();
	cout << message.str();
	message.str("");
	m_write.unlock();
}

// Преобразование числа в доктора.
string getDoctor(int index) {
    switch (index) {
        case 1:
            return "Dentist";
        case 2:
            return "Surgeon";
        case 3:
            return "Therapist";
        default:
            return "";
    }
}

// Ввод пользователем числа, с повторным вводом, при некорректных входных данных.
int getCountOfPatients() {
    // Цикл продолжается до тех пор, пока пользователь не введет корректное значение.
    while (true) {
        cout << "Введите количество пациентов: ";
        int a;
        cin >> a;

        // Проверка на предыдущее извлечение
        if (cin.fail() || a <= 0) { // Если предыдущее извлечение оказалось неудачным
            cin.clear(); // то возвращаем cin в 'обычный' режим работы
            cin.ignore(32767,'\n'); // и удаляем значения предыдущего ввода из входного буфера
            cout << "Oops, that input is invalid.  Please try again.\n";
        }
        else {
            cin.ignore(32767,'\n'); // удаляем лишние значения
            return a;
        }
    }
}

// Структура информации  о пациенте.
struct {
    int id; // Номер пациента.
    int doctor; // Тип его доктора.
}info;

// Искусственная задержка для правдободности программы.
void sleep(int ms) {
    this_thread::sleep_for(chrono::milliseconds(ms));
}

void doctor() {
    unique_lock<mutex> lexchange1(exchange1);
    unique_lock<mutex> lexchange2(exchange2);

    stringstream ss;

    while (!finish) {

        bserverready = true;
        cserverready.notify_one();

        while (!bexchange1) { cexchange1.wait(lexchange1); }

        bexchange1 = false;

        ss << getDoctor(info.doctor) << " started to heal patient " << info.id << "." << endl;
        write(ss);

        sleep(1000 + rand() % 10000);

        ss << getDoctor(info.doctor) << " finished healing patient " << info.id << "." << endl;
        write(ss);

        bserveranswer = true;
        cserveranswer.notify_one();

        while (!bexchange2) { cexchange2.wait(lexchange2); }

        bexchange2 = false;
        sleep(100);
    }
}

void patient(int index, int doctor) {
    unique_lock<mutex> lserverready(serverready);
    unique_lock<mutex> lserveranswer(serveranswer);

    srand(time(nullptr));

    stringstream ss;

    ss << "Patient " << index << " came to the hospital. " << endl;
    write(ss);

    sleep(1000 + rand() % 10000);

    ss << "Patient " << index << " going to the " << getDoctor(doctor) << "." << endl;
    write(ss);

    sleep(1000 + rand() % 10000);

    while (!bserverready) { cserverready.wait(lserverready); }
    bserverready = false;

    ss << "Patient " << index << " came to the " << getDoctor(doctor) << "." << endl;
    write(ss);

    info.id = index;
    info.doctor = doctor;

    bexchange1 = true;
    cexchange1.notify_one();

    while (!bserveranswer) {
        cserveranswer.wait(lserveranswer);
    }

    bserveranswer = false;
    ss << "Patient " << info.id << " cured." << endl;
    write(ss);

    bexchange2 = true;
    cexchange2.notify_one();
    ss << "Patient " << index << " left hospital." << endl;
    write(ss);
}

int main() {
    // Поддержка кириллицы.
    setlocale(LC_ALL, "Russian");

    int count = getCountOfPatients();

    thread doctor1(doctor, 1);

    vector<thread> patients(count);

    for (int i = 0; i < count; i++) {
        patients.push_back(thread(patient, i, rand() % 3));
    }

    for (int i = 0; i < count; i++) {
        patients.at(i).join();
    }

    doctor1.join();

    return 0;
}
