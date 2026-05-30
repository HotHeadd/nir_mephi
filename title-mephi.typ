#import "@preview/modern-g7-32:0.2.0": custom-title-template
#import custom-title-template: *

#let arguments(..args, year: auto) = {
  let args = args.named()
  args.organization = fetch-field(
    args.at("organization", default: none),
    ("*full", "short"),
    hint: "организации",
  )
  args.manager = fetch-field(
    args.at("manager", default: none),
    ("position*", "name*"),
    hint: "руководителя",
  )
  return args
}

#let unbreak-name(name) = {
  if name == none { return }
  return name.replace(" ", "\u{00A0}")
}

#let custom-sign-field(name, position, label: none, part: none, details: "подпись, дата", is-student: false) = {
  let part-cell = []
  if part != none {
    part-cell = table.cell(align: top)[(#small-text[#part])]
  }
  let position-text = if is-student { "Группа " + position } else { position }
  set par(justify: false)
  table(
    stroke: none,
    inset: (x: 0pt, y: 3pt),
    columns: (5fr, 1fr, 3fr, 1fr, 3fr),
    table.cell(colspan: 5)[#if label != none { label }],
    [#position-text], [], [], [], table.cell(align: bottom)[#unbreak-name(name)],
    [], [], table.cell(align: center)[], [], part-cell
  )
}


#let template(
  ministry: none,
  university: none,
  department: none,
  step-number: none,
  subject: none,
  group: none,
  manager: (position: none, name: none),
  student: (group: none, name: none),
  city: none,
  year: auto,
  ..rest,
) = {
  align(center)[
    #upper(ministry)
    #v(0.15fr)
    #university
    #v(0.15fr)
    #department
    #v(1fr)
    #text([Отчет о научно-исследовательской работе\ по этапу №#step-number])
    #v(0.2fr)
    #text([по темe:])
    #linebreak()
    #upper([#subject])
    #v(0.4fr)
    #v(1fr)
  ]
  custom-sign-field(student.name, student.group, label: "Студент:", is-student: true)
  v(0.1fr)
  custom-sign-field(manager.name, manager.position, label: "Научный руководитель:")
  v(0.6fr)
}