# PING-PONG

### Требования
Python 3.7+

## Запуск
Исполняемый скрипт находится в папке `net`:
`python -m net.cli <команда> ...`

---

1) TCP сервер  
(обслуживает одного клиента за раз, эхо + двунаправленный ввод/вывод)
```bash
python -m net.cli tcp-server --host 0.0.0.0 --port 50000
````

2. TCP клиент — **интерактивный**
   (связывает stdin/stdout с сокетом)

```bash
python -m net.cli tcp-client --host 127.0.0.1 --port 50000 --interactive
```

3. TCP клиент — **одиночное сообщение + эхо-ответ**

```bash
python -m net.cli tcp-client --host 127.0.0.1 --port 50000 --message "hello"
```

---

4. UDP сервер
   (получает датаграммы и отвечает эхо)

```bash
python -m net.cli udp-server --host 0.0.0.0 --port 50000
```

5. UDP клиент — одиночное сообщение

```bash
python -m net.cli udp-client --host 127.0.0.1 --port 50000 --message "hi"
```

UDP клиент может читать stdin, если сообщение не указано:

```bash
echo "hi" | python -m net.cli udp-client --host 127.0.0.1 --port 50000
```

6. UDP клиент — **интерактивный режим**

```bash
python -m net.cli udp-client --host 127.0.0.1 --port 50000 --interactive
```

---

## Совместимость с `netcat`

Может использоваться для отладки:

```bash
nc 127.0.0.1 50000       # TCP клиент к tcp-server
nc -u 127.0.0.1 50000    # UDP клиент к udp-server
```

---

## Тест на большую строку (~50 KB)

```bash
python -m net.cli test-huge --host 127.0.0.1 --port 50000 --size 50000
```

---

## Описание поведения

**TCP сервер**

* принимает одного клиента
* эхо-ответы
* выводит полученные данные в stdout
* умеет читать stdin и отправлять клиенту (двухсторонняя работа)
* после отключения клиента снова ждёт нового

**UDP сервер**

* получает датаграмму
* немедленно отправляет обратно тот же payload
* выводит принятые данные в stdout