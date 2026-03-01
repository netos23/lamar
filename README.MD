# Lamar
[![CI](https://github.com/netos23/lamar/actions/workflows/ci.yml/badge.svg)](https://github.com/netos23/lamar/actions/workflows/ci.yml)
---
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

- Анализ идиом (анализ с помощью `lamar_analyzer`):

```bash
test/run_analyzer_perf.sh
```

  Переменные окружения: `BC_DIR` (каталог с *.bc, по умолчанию `performance/`), `ANALYZER_BIN` (по умолчанию `cmake-build-release/lamar_analyzer`).

  Скрипт `test/run_analyzer_perf.sh` перебирает все `.bc` из `BC_DIR`, запускает `lamar_analyzer` и выводит сводку по ошибкам.

При необходимости на ARM/Mac можно воспользоваться контейнером из `docker/README.MD`.

## Вспомогательные цели и режимы

### Сборка анализатора и дизассемблера
Сборки `lamar_analyzer` и `lamar_disassembler` используют те же исходники и рантайм, что и основной интерпретатор.
Чтобы подготовить бинарники для анализа идиом и дизассемблирования, запустите:

```bash
cmake --build cmake-build-release --target lamar_analyzer
cmake --build cmake-build-release --target lamar_disassembler
```

`lamar_analyzer` нужен для `test/run_analyzer_perf.sh`, а `lamar_disassembler` — для `test/run_bc_compare.sh` и собственных сценариев дизассемблирования.

### Верификация и измерения времени

`ENABLE_VERIFICATION` включает вызов `lamar::Verifier`, а `MESHURE_TIME` выводит измерения времени выполнения (верификация — в секундах, интерпретация — в секундах).
Все макросы задаются компилятору вручную, например:

```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-DENABLE_VERIFICATION -DMESHURE_TIME -DDISABLE_RUNTIME_CHECKS"
cmake --build cmake-build-release
```

### Итог
- `DISABLE_RUNTIME_CHECKS` убирает все проверки в `Interpreter`, ускоряя обход байткода, но делает ошибки на входе фатальными.
- `ENABLE_VERIFICATION` сначала запускает `lamar::Verifier`, чтобы поймать ошибки  до исполнения.
- `MESHURE_TIME` полезен на этапе сравнения оптимизаций и показывает, сколько времени уходит на верификацию и интерпретацию подряд.

### Сравнение с другими интерпретаторами

| Интерпретатор   | Время выполнения (сек) | Score~ |
|-----------------|------------------------|--------|
| lamar-verified* | 105.12                 | -      |
| lamar*          | 114.84                 | +9%    |
| lamac -s        | 171.67                 | +50%   |
| lamac -i        | 574.08                 | +234%  |

\* Mac OS m1 16GB RAM with Rosseta 2, другие измерения на Github Actions (Linux, 4 vCPU, 16GB RAM).
