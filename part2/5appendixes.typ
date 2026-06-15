#import "@preview/modern-g7-32:0.2.0": appendixes
#show: appendixes

#show figure.where(kind: raw): set block(breakable: true)

= Исходный код агента-наблюдателя

#figure(
  raw(read("code/src/agent.c"), lang: "c", block: true),
  caption: [Главный цикл наблюдения (agent.c)],
) <listing-agent>

#figure(
  raw(read("code/src/inject.c"), lang: "c", block: true),
  caption: [Инжекция системного вызова mprotect (inject.c)],
) <listing-inject>

#figure(
  raw(read("code/src/guard.c"), lang: "c", block: true),
  caption: [Установка и восстановление охраны области (guard.c)],
) <listing-guard>

#figure(
  raw(read("code/src/attrib.c"), lang: "c", block: true),
  caption: [Атрибуция события к модулю и функции (attrib.c)],
) <listing-attrib>