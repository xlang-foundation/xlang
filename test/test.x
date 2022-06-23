"""File Name: test.json """ 
x = {
	"glossary": {
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
					"GlossSee": "markup"
				}
			}
		}
	}
} 
print(x["glossary"]["GlossDiv"]["GlossList"]["GlossEntry"]["Glossdef"])
