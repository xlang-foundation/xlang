x="012ABCDabcd"
x.each(@constexpr(${0}>='a' and ${0}<='z',
        ${0}>='A' and ${0}<='Z'),
        @constexpr(${0}>='A':${0}-=('A'+'a')))
print("end")
