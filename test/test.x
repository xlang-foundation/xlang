"""File Name: test.json """ 
s = "var"
print(s)
x = {
	"glos sary": {
		"GlossSeeAlso": [ "GML", "XML" ],
		"title": "example glossary",
		"GlossDiv": {
			"title": "S",
			"GlossList": {
				"GlossEntry": {
					"ID": "SGML",
					"SortAs": "SGML",
					"GlossTerm": "Standard
					Generalized Markup
					Language",
					"Acronym": "SGML",
					"Abbrev": "ISO 8879:1986",
					"Glossdef": {
						"para0": "A meta-markup language, used to create markup languages such as DocBook.",
						"GlossSeeAlso": [ "GML", "XML" ]
					},
					"GlossSee": "markup",
					1:[1,2,3,"4+5"]
				}
			}
		}
	}
} 
y = x["glos sary"]["GlossDiv"]["GlossList"]["GlossEntry"]
print("x lookup:",y)
