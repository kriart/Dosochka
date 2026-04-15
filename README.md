# Dosochka

Многопользовательская онлайн-доска в духе Miro:
- сервер на `Boost.Asio` и `Boost.JSON`
- клиент на `Qt Widgets`
- бизнес-логика в отдельных `libs`
- поддержка `in-memory` и `PostgreSQL` persistence

## Структура проекта

- `libs/common` — базовые типы и инфраструктурные примитивы: `ids`, `error`, `result`, `clock`
- `libs/domain` — доменная модель доски, пользователей и операций
- `libs/application` — use-case слой, DTO и интерфейсы репозиториев
- `libs/runtime` — runtime-менеджеры активных досок
- `libs/persistence` — `in-memory` и `PostgreSQL` реализации репозиториев
- `libs/protocol` — общие типы сообщений протокола
- `apps/server` — серверное приложение
- `apps/client-qt` — клиентское Qt-приложение
- `tests` — unit- и Qt-тесты
- `docs` — планы, спецификации и промежуточные обзоры архитектуры

## CMake

Сборка разнесена по подпроектам:
- корневой [CMakeLists.txt](C:/Users/artem/Downloads/Dosochka/CMakeLists.txt) задает политики, зависимости и подключает подпапки
- `libs/*/CMakeLists.txt` описывают внутренние библиотеки
- `apps/*/CMakeLists.txt` описывают сервер и клиент
- [tests/CMakeLists.txt](C:/Users/artem/Downloads/Dosochka/tests/CMakeLists.txt) описывает тестовые таргеты

## Зависимости

Основные зависимости:
- `Boost` (`asio`, `json`, `system`) для сервера
- `Qt6` (`Core`, `Widgets`, `Network`, опционально `Test`) для клиента
- `PostgreSQL` и `libpqxx` для postgres-бэкенда
- `GoogleTest` для unit-тестов

## Сборка

### Базовая сборка

```powershell
cmake -S . -B build-vcpkg -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_PREFIX_PATH=C:\Qt\6.11.0\msvc2022_64
cmake --build build-vcpkg --config Debug --target online_board_server online_board_client_qt online_board_tests online_board_client_qt_tests
ctest --test-dir build-vcpkg -C Debug --output-on-failure
```

### Запуск с PostgreSQL

Поднять БД:

```cmd
docker compose -f C:\Users\artem\Downloads\Dosochka\docker-compose.yml up -d postgres
```

Запустить сервер:

```cmd
set DOSOCHKA_DATABASE_URL=host=127.0.0.1 port=55000 dbname=dosochka user=dosochka password=dosochka && C:\Users\artem\Downloads\Dosochka\build-vcpkg\Debug\online_board_server.exe
```

Запустить клиент:

```cmd
C:\Users\artem\Downloads\Dosochka\build-vcpkg\Debug\online_board_client_qt.exe
```

### Запуск без PostgreSQL

Сервер:

```cmd
C:\Users\artem\Downloads\Dosochka\build-vcpkg\Debug\online_board_server.exe
```

Клиент:

```cmd
C:\Users\artem\Downloads\Dosochka\build-vcpkg\Debug\online_board_client_qt.exe
```

## Текущее состояние

Сейчас реализовано:
- регистрация, логин, guest-вход и восстановление сессии
- создание и вход на доски
- роли и политики доступа
- синхронизация операций между несколькими клиентами
- рисование `pen`, `rectangle`, `text`
- ластик по `drag`
- pan, zoom, выбор цвета и толщины линии
- список активных участников
- `in-memory` persistence и `PostgreSQL` persistence
- unit- и Qt-тесты

## Документация

Основные проектные материалы лежат в [docs](C:/Users/artem/Downloads/Dosochka/docs):
- планы этапов
- спецификация серверной бизнес-логики
- функциональные требования
- промежуточный архитектурный обзор
