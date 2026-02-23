# Lamar

Интерпретатор байткода языка LaMa.

---
## Оглавление
- [Навигация](#навигация)
- [Зависимости](#зависимости)
- [Сборка](#сборка)
- [Запуск](#запуск)
- [Тесты](#тесты)
- [Сравнение с другими интерпретаторами](#Сравнение-с-другими-интерпретаторами)
---
## Навигация
- [Docker-окружение для ARM/Mac](docker/README.MD)
- [Утилиты и генерация фикстур](scripts/README.MD)

## Зависимости
- `cmake` \>= 3.29
- Компилятор с поддержкой C++17 (clang или gcc)
- `make`/`ninja` (любой генератор, по умолчанию используется cmake)
- Git для загрузки сабмодуля рантайма

Инициализация сабмодуля (обязательно перед сборкой):

```bash
git submodule update --init --recursive
```

На macOS необходимые инструменты можно поставить через Homebrew: `brew install cmake ninja`.

## Сборка
Релизная сборка (используется по умолчанию в IO-тестах):

```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release
```

Отладочная сборка с санитайзерами (используется в тесте сравнения дизассемблера):

```bash
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug
```

После сборки бинарник находится в `cmake-build-*/lamar`.

## Запуск
Базовый запуск интерпретатора:

```bash
cmake-build-release/lamar path/to/program.bc
```

Полезные опции:
- `--print-disassemble` — печать дизассемблированного кода в stdout;
- `--disassemble-file=<path>` — сохранить дизассемблированный код в файл;
- `--disassemble-only` — не запускать байткод после дизассемблирования.

## Тесты
В регрессионных тестах используются байткоды и эталонные входы/выходы из каталога `regression/`.

- Сравнение вывода дизассемблера с эталонным `byterun` (требует `cmake-build-debug` и собранный сабмодуль `third_party/Lama/byterun`):

```bash
test/run_bc_compare.sh
```

  Переменные окружения: `BC_DIR` (каталог с *.bc), `BYTERUN_BIN`, `LAMAR_BIN`.

- IO-регрессии (сравнение stdout с *.t при подаче *.input):

```bash
test/run_bc_io.sh
```

  Переменные окружения: `BC_DIR`, `LAMAR_BIN` (по умолчанию `cmake-build-release/lamar`).

- Производительность (измерения времени `lamar`/`lamac` на файлах из `performance/`):

```bash
test/run_performance.sh
```

  Переменные окружения: `BC_DIR` (каталог с *.bc, по умолчанию `performance/`), `LAMAR_BIN` 
  (по умолчанию `cmake-build-release/lamar`), `LAMAC_BIN` (по умолчанию `lamac`, можно передать путь).

При необходимости на ARM/Mac можно воспользоваться контейнером из `docker/README.MD`.


### Сравнение с другими интерпретаторами

| Интерпретатор | Время выполнения (сек) | Score~ |
|---------------|-----------------------|--------|
| lamar*        | 114.84                | -      |
| lamac -s      | 171.67                | +50%   |
| lamac -i      | 574.08                | +234%  |

* Mac OS m1 16GB RAM with Rosseta 2, другие измерения на Github Actions (Linux, 4 vCPU, 16GB RAM). 