# SuperChain

A simple tool that lists the same-named and same-typed fields in the subclass and superclass inside an APK file.


The following fields are ignored:

- static

- private

- synthetic (automatically generated by the compiler)


How to compile: 

```shell

git clone git@github.com:nlifew/SuperChain.git

cd SuperChain

mkdir build & cd build

cmake ..

make

```

Then you will found SuperChain executable file.


Usage:

```shell

./SuperChain [apk file]

```





一个简单的小工具，扫描 apk 内所有类，并列出父类和子类的同名同类型字段。

以下类型的字段会被忽略掉：

- static

- private

- synthetic (编译器自动生成的)


编译方式

```shell

git clone git@github.com:nlifew/SuperChain.git

cd SuperChain

mkdir build & cd build

cmake ..

make

```

然后就能在当前目录下找到 SuperChain 可执行文件了


使用方式

```
./SuperChain [apk file]
```


