# ThreadPool
## Описание концепции
Класс, представляющий пул потоков, способных выполнять различные задачи.

Концептуально представляет собой одну очередь, куда складываются клиентские задачи, и несколько потоков, которые эту очередь разгребают, выполняя задачи.

Когда клиент кладет задачу в очередь, он получает объект `std::future`, с помощью которого он сможет потом достать результат выполнения задачи.

В качестве задач можно скармливать функторы, лямбды, указатели на функции, объекты `std::function` и пр.

Задачи могут возвращать значения или возвращать `void` -- в этом случае из `std::future` невозможно достать результат, его можно только дождаться.

## Пример использования
```
ThreadPool pool(5);                                             // Создаем пул из пяти рабочих потоков

auto future1 = pool.AddTask([](int x) { return x + 10; }, 3);   // Отдали задачу -- получили std::future
auto answer1 = future1.get();                                   // Получили ответ из синхронного std::future::get()

auto future2 = pool.AddTask([] { std::cout << "hello"; });      // Отдали задачу -- получили std::future
future2.get();                                                  // Убедились, что задача выполнена
```

## Часто задаваемые вопросы по `AddTask()` и `WorkerThread()`

Q. А если `foo()` выполнится раньше, чем мы вернем `std::future` клиенту?</br>
A. Когда `foo()` выполнится, она проставит ответ в `std::packaged_task`. Будучи в методе `AddTask()`, мы просто возьмем `std::future` от `std::packaged_task` с уже проставленным ответом и отдадим клиенту.

Q. А если `foo()` выполнится, когда мы выйдем из `AddTask()` -- тогда уничтожится `std::packaged_task`. Откуда возьмется результат?</br>
A. Так как `std::packaged_task` упакован в `std::shared_ptr`, то он будет жить, пока на него есть ссылки. Когда мы формируем `foo()`, она добавляет новую ссылку на `std::packaged_task`. Поэтому даже когда мы выйдем из `AddTask()`, `std::packaged_task` останется жить на куче, пока не выполнится `foo()`. Когда `foo()` выполнится, у нас к тому моменту уже будет `std::future`, в который `std::packaged_task` перед смертью проставит ответ.

Q. А вот если `foo()` выполнится и сразу же перезатрется следующей `foo()` из очереди -- не страшно?</br>
A. Не `foo()` возвращает ответ клиенту, а `std::packaged_task` через `std::future`. `foo()` нужна только для того, чтобы выполнить тело `std::packaged_task`, чтобы она отдала ответ `std::future`. Каждая `foo()` в очереди "настроена" на свою `std::packaged_task`. Поэтому как только `foo()` выполнилась, она больше не нужна и смело может быть перезатерта.
