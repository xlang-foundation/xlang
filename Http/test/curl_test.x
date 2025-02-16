from xlang_http import http

c = http.Curl()
c.setOpt("URL","https://www.oxfordlearnersdictionaries.com/us/definition/english/transcend?q=transcending")
# Set a couple of options.
c.setOpt(c.FOLLOWLOCATION, True)
c.setOpt("SSL_VERIFYPEER", False)
c.setOpt("SSL_VERIFYHOST", False)
c.setOpt("TIMEOUT", 10)
# Optionally add a custom user-agent.
c.setOpt(c.USERAGENT, "XLang-Curl-Client/1.0")
if c.perform():
    print("Response:", c.response)
else:
    print("Error:", c.error)

print("Done")

