# What's this?

This is a fork of a branche of Phobos,which bases on CrimRecya's and MJobos‘s Phobos branch.

这是Phobos平台的一个分支的分支，基于绯红热茶和航味麻酱的Phobos分支。

The reason for the existence of this branch is that CrimRecya and MJobos are reluctant to merge some Pull requests, so I forked it out for me to merge Pull requests.

此分支存在的意义是：绯红热茶和航味麻酱不愿意合并某些PR，所以我将它分叉了出来，以供我合并PR。

I'll follow up with the Phobos branch of the original CrimRecya and MJobos, and here are just a few features that go beyond that branch.

我会跟进原绯红热茶和航味麻酱的Phobos分支，此处仅列出比那个分支多出来的功能。

# New：

1.A new script action for AI repairing destroyed bridges.(#791 by FS-21)

Picks a Bridge Repair Hut from the map that is linked with a bridge with destroyed sections and is reachable by engineers and then send the Taskforce against it.The Taskforce must have any engineer and (Bridge Repair Huts in the map) or the script will end.Puts non-engineers into Area Guard mode when they arrive near the Bridge Repair Hut location.Only are evaluated Bridge Repair Huts that can be reached by the Engineers.

In `aimd.ini`:

```ini
[SOMESCRIPTTYPE] ; ScriptType
x=10105,n ; integer, mode for selecting Bridge Repair Huts
;The possible argument values are:
;0 -> Pick the closest
;1 -> Pick the Farthest
;-1 -> Pick Random
```

1.专用于AI修复桥梁的脚本（Script action for repairing destroyed bridges #791 by FS-21）

使用此脚本，AI的单位可以寻找地图上连接了损坏桥梁的桥梁维修小屋进行维修，该特遣部队必须有工程师，且地图中必须存在桥梁维修小屋，否则脚本将结束。 当特遣部队中存在非工程师单位并到达桥梁维修小屋附近时，他们将会进入区域警戒（Area Guard）状态。

在`aimd.ini`中：

```ini
[SOMESCRIPTTYPE] ;脚本类型
x=10105,n ;n为整数型，AI工程师选择桥梁维修小屋的模式
;n的值可以取：
;0  -> 选择最近的
;1  -> 选择最远的
;-1 -> 随机选取
```
