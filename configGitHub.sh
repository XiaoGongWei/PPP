sudo apt-get install git
#你的github用户名
git_name=${1}
#你的github邮箱地址
git_email=${2}
git config --global user.name "${1}"  
git config --global user.email "${2}" 

ssh-keygen -t rsa -C "${2}"

echo "Copy the following string to <github.com: Settings -> SSH Keys -> Add SSH Key>"
cat ~/.ssh/id_rsa.pub
read -p "input yes:" -t 5 x_input

ssh -T git@github.com

"""
cd ~/Document/newflode  
git init  
touch Readme.me

git remote add origin https://github.com/XiaoGongWei/Ubuntu16.04-Code.git

git add *
git commit -m 'add some files'
git push origin master

vim .git-credentials
https://{username}:{password}@github.com
git config --global credential.helper store

"""
