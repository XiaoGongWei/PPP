echo ${1}
git rm --cached -r ${1}
git commit -am "remove file: ${1}."
git push origin master 
