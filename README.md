# Сервис url-shortener
Сервис предоставляет пользователям возможность делать короткие ссылки. Переходя по короткой ссылке, созданной в сервисе, браузер перенаправляется на оригинальный адрес.


### Установка

1. Склонируйте репозиторий `git clone your-service-repo && cd your-service-repo`
2. Обновите сабмодули `git submodule update --init`
3. Запустите сборку
- В docker контейнере `make docker-build-debug` (рекомендуется)
- Локально `make build-debug` (не рекомендуется)


## Запуск приложения
1. Запустите сервис командой `make docker-start-service-debug`
2. Отправьте запрос на сокращение ссылки с помощью утилиты командной строки curl.
```
curl -X POST localhost:8080/v1/make-shorter \
  -d '{
    "url": "https://ya.ru"
  }'
```
В ответ придет примерно такой JSON:
```
{
	"short_url": "http://localhost:8080/some-id"
}
```
Где `short_url` - адрес, переход по которому, осуществит перенаправление запроса на оригинальный адрес.
3. Откройте короткий URL `http://localhost:8080/some-id` в браузере и запрос будет перенаправлен на оригинальный URL.

## Задание

На лекции мы проследили за процессом, как разрабатывать новые крутые продуктовые фичи в нашем сервисе Лавка. Давайте реализуем что-то похожее на небольшом проекте UrlShortener. Он всего лишь умеет укорачивать урлы и дает немного управления над созданными редиректами (можно посмотреть статистику использования и удалить ссылку).

Представьте, что нам необходимо вывести наш сервис UrlShortener на новый качественный уровень на рынке подобных систем. Надо завоевать долю на рынке и нарастить количество активных пользователей с X до Y %. Это стратегическая цель. Но как ее достичь?

### Идея фичи

Ваш коллега из продуктовой команды проанализировал рынок и решил, что killer-фичей, которую так необходимо реализовать во что бы то ни стало, является "VIP ссылки"!

Что это? Всё просто. Вы помните, что UrlShortener возвращает произвольную комбинацию символов в укороченном URL? А в VIP ссылках, это не так: пользователь сам указывает, какой будет его короткая ссылка, конечно, только если заданная им комбинация символов свободна.

Формальное описание интерфейса на OpenAPI 3.0 тут [src/main/resources/url_shortener_iface.yaml](src/main/resources/url_shortener_iface.yaml)

А ниже для общего представления неформальное описание.

make_shorter на входе получает:
- longUrl = "user-defined-long-url"
- optional vipKey = "user-defined-symbols"
- optional timeToLive = 1
- optional timeToLiveUnit = SECONDS, MINUTES, HOURS, DAYS

Максимальный TimeToLive не должен превышать 2 дня (иначе красивые vip ссылки закончатся).

В ответ на операцию приходит:

- shortUrl = "example.com/xyz" - короткая ссылка
- secretKey - ключ для управления ссылкой

Ну, или ошибка 400, если есть какие-то проблемы с входными параметрами, например, если vipKey уже занят или переданы невалидные значения для TTL.

Тут надо обратить внимание на то, что новая функциональность встраивается в уже имеющийся продукт, при этом важно не нарушить старого поведения системы нововведениями с TTL.

### Моделирование жизни фичи
А теперь давай представим, что у нашего сервиса UrlShortener есть frontend часть и реальные пользователи. Как бы велась разработка новой фичи в этих условиях? Какие эксперименты можно провести, чтобы удостовериться, что пользователи оценят внедренную идею? За какими продуктовыми метриками стоит следить при проведении эксперимента?

Всё это непростые вопросы, на которые надо отвечать при разработке и оценке новой идеи. В следующем разделе с требованиями к результатам работы нужно будет изложить свое видение ответов.

### Требования к результату домашней работы
Тебе выдан форк репозитория сервиса UrlShortener. 

Изменения в коде нужно сделать через Merge Request в **свой репозиторий**. Таким образом для разработки отводится новая ветка от `master`, и по ней создается Merge Request. 

Для этого задания предусмотрена автоматическая проверка в LMS, которая запускает тесты из ветки мастер и вычисляет количество заработанных баллов.

На примере одного из студенческих решений на разборе проверим как можно было реализовать задание. Провалидируем получившиеся продуктовые гипотезы, рассмотрим реализацию фичи в коде, дизайн AB-эксперимента, продуктовые метрики.

В идеале мы хотим, чтобы студент проделал следующую работу:
1. Описал идею в 1-2 предложениях
3. Сформулировал продуктовые гипотезы: 1-2 штуки в формате `если <действие> то <следствие> потому что <объяснение>`
4. Оценил примерные трудозатраты на полноценную реализацию в бэкенде (попробуй аргументировать оценку, например, требуемым количеством новых классов, объемом кода)
5. Придумал, можно ли сделать MVP, если да, то как оно будет выглядеть и сколько это займет времени
6. Проработал архитектуру и описал ее в тексте (в качестве формального описания подойдут диаграммы классов, компонентные диаграммы, диаграммы последовательности UML - в зависимости от того, что лучше отразит суть изменений и, что по вашему мнению будет понятнее проверяющему)
7. Реализовал полное решение идеи в коде. Это та часть работы, которая будет проверена автотестированием.
8. Добиться, чтобы все предоставленные в исходном репозитории тесты на vip-ссылки проходили успешно, внося правки в код сервиса, а не тестов :) 
Кстати, тесты не работают одновременно с запущенным приложением (один порт).
9. Придумал АБ-тест: какие выборки пользователей будут в эксперименте? Какие параметры фичи будем проверять в каждой выборке?
10. Выбрал и описал набор наблюдаемых продуктовых метрик, по которым можно сделать вывод, что идея "взлетела". Какие значения метрик ожидаем увидеть?
11. Сделал отчет в файле design.md в своем репозитории с ответами на эти вопросы.


### А что, если у меня своя классная продуктовая идея?
Если есть своя идея, как вырастить счастье пользователей UrlShortener, то можно реализовать и её (дополнительно). 

Это необязательное задание, но оно будет оценено дополнительными баллами для тех, кто презентовал свою идею на встрече по разбору домашнего задания. 

Что нужно сделать: 
* Собственную идею необходимо разрабатывать в отдельной ветке в своем форке репозитория
* Описать путь твоей фичи от идеи до оценки эффекта ее внедрения по алгоритму, описанному в требованиях к работе: продуктовые гипотезы, план и оценка трудозатрат, MVP, архитектура, реализация (тестирование можно опустить), АБ-тесты, метрики.
* Записать короткий (до 2 минут) видео ролик с презентацией своей идеи.
* Презентовать свою идею на встрече по разбору домашнего задания.

## Работа с приложением

### Требования

Необходимо, чтобы были установлены следующие компоненты:

- `Docker` и `docker-compose`
- `Python`

### Установка

1. Склонируйте репозиторий `git clone your-service-repo && cd your-service-repo`
2. Обновите сабмодули `git submodule update --init`
3. Запустите сборку
- В docker контейнере `make docker-build-debug` (рекомендуется)
- Локально `make build-debug` (не рекомендуется)

### Запуск

- Запуск приложения в docker контейнере (рекомендуется):
```commandline
make docker-start-service-debug
```

- Запуск приложения локально (не рекомендуется):
```commandline
make service-start-debug
```

### Тестирование

- Запуск тестов в docker контейнере (рекомендуется):
```commandline
make docker-test-debug
```

- Запуск тестов локально (не рекомендуется):
```commandline
make test-debug
```

### Запуск форматирования кода
```commandline
make format
```

### Попробнее про фреймворк userver

- Документация
https://userver.tech/index.html

- Исходный код
https://github.com/userver-framework/userver

- Шаблон сервиса
https://github.com/userver-framework/service_template

- Шаблон сервиса с базой
https://github.com/userver-framework/pg_service_template

- Полный список возможных команд сервиса
https://github.com/userver-framework/pg_service_template?tab=readme-ov-file#makefile

### Работа со временем

При работе используйте `userver::utils::datetime::Now` из `<userver/utils/datetime.hpp>`. Если спользовать просто `chrono`, то не будет работать `mocked_time` в тестах
