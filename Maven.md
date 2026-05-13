# Maven

话说到底什么是maven呢？为什么要使用？

是这样理解的，在写一个项目的时候，有很多代码是基础的重复性的，并且很多，所以这个代码就会被打包称jar压缩包，每次使用的时候下载jar包就可以直接使用了

但是一个项目一个系统的jar包实在是太多了，所以就有了maven这个东西，它可以帮你自动匹配对应版本的jar包，放入对应目录，直接就能跑---很方便，感觉像是自动化时代一样

在构建的时候呢——你写的java,但是服务器运行的是jar或者war,这个时候maven就会自动帮你转变成jar包等

项目周期——maven有一套完整的流程，并不是只有运行

多模块管理————有很多项目共同组建而成比较方便

还有很多插件系统，全部自动化，自动打包，自动生成代码～～

## 第一课

理解了maven生成的pom.xml

```
<project>

    <modelVersion>4.0.0</modelVersion>

    <groupId>com.demo</groupId>
    <artifactId>hello-maven</artifactId>
    <version>1.0</version>

</project>
```

就是让maven知道公司名，项目名，版本号，安装什么jar只需要在这里面写上需求maven就会自动帮助你去下载

## 第二课Maven 生命周期（真正理解 clean / package / install）

话说，写一个java文件电脑是不能直接运行的，它需要变成class才能运行

之前编译运行可能是输入javac java等，但是当java文件过多的时候你就不鞥一个一个的输入了，这时候maven出现了

maven有一个自己的完整的项目构建流程

```
清理clean
编译compile
测试test
打包package
安装install
部署
```

这也就是它的生命周期

### clean

`mvn clean`就是删除1上一次的构建结果，有时候你需要重新编译运行的时候需要删除之前生成的target目录，里面存放者编译后的class,打包的jar,临时文件等，他们会影响新一次的构建，所以要清理干净

### compile

`mvn compile`编译java源码，也就是变成class文件

### test

`mvn test`自动运行测试代码，maven会自动测试所有代码

### package

`mvn package`就是把项目打包成jar或者war,方便服务器运行

### install

为什么要install,其实就是上传到本地仓库，其他项目就能够引用它

`mvn install`==package+放进本地仓库

### 最关键的是

> 执行后面的，会自动执行前面的

所以在公司里面，基本上都直接写`mvn clean install`

先删除干净，然后编译测试打包安装

## 第三课dependency 到底是什么？

你需要使用外部的jar包

### import

导入关键字，也就是说我要用其他人写的代码了，但是为什么可以import成功呢？因为项目里面已经有jar包了

### 那么依赖到底是什么？

例如以下的

```
<dependency>
    <groupId>mysql</groupId>
    <artifactId>mysql-connector-java</artifactId>
    <version>8.0.33</version>
</dependency>
```

我要下载8.0.33的mysql的jar包

而maven就会先去仓库寻找项目名——然后下载jar包——放在本地的仓库里面——自动加入到项目目录里——这样就可以直接导入import了

### 传递依赖

加入你需要A包，但是A包需要其他包，这时候maven会自动下载其他的BCD包，这就是传递依赖

### 依赖冲突

大概就有些包的版本不统一而冲突了，项目不知道使用哪一个，而maven的难点就是依赖关系，包太多，他们的关系也很复杂

### dependency scope

scope 意思：

> “依赖在什么阶段生效”

1.默认是compile——编译运行测试

2.test——只在测试的时候使用

3.provided——运行环境已经提供了

```
只编译、不打包、不带走
我写代码要用
但运行环境已经有了
别打进 jar，避免冲突、瘦身
```

4.runtime——表示编译时不用，但是运行时需要