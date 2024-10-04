## What's this?

This is a fork of a branche of Phobos,which bases on CrimRecya's and MJobos‘s Phobos branch.

这是Phobos平台的一个分支的分支，基于绯红热茶和航味麻酱的Phobos分支。

The reason for the existence of this branch is that CrimRecya and MJobos are reluctant to merge some Pull requests, so I forked it out for me to merge Pull requests.

此分支存在的意义是：绯红热茶和航味麻酱不愿意合并某些PR，所以我将它分叉了出来，以供我合并PR。

I'll follow up with the Phobos branch of the original CrimRecya and MJobos, and here are just a few features that go beyond that branch.

我会跟进原绯红热茶和航味麻酱的Phobos分支，此处仅列出比那个分支多出来的功能。

## New：

# 1.专用于AI修复桥梁的脚本（Script action for repairing destroyed bridges #791 by FS-21）

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

# 2. 新的 AI Teams 选择器（New AI Teams Selector #810 by FS-21）

- New AI system for selecting valid triggers in multiplayer randomly. Unlike the original method this one checks prerequisites and take care of other details.
- It can split the valid triggers into 4 categories: ground, air, naval & mixed categories. If set, AI picks a random trigger in a random category.
- Categories can have different chance probabilities. It can be set globally or customized per house. by default each category has a 25% chance to be selected.
- `NewTeamsSelector.MergeUnclassifiedCategoryWith` can be used for merging the mixed category (units are from different categories) into one of the main categories.
- In case of picking a category without valid triggers exist a fallback mode that allow picking a trigger from all valid triggers like if categories were disabled.
- if `Autocreate=yes` AI will care about all units prerequisites so if the house's tech tree is incomplete for the trigger it gets discarded. It understand Ares tags like  `Prerequisite.RequiredTheaters`, `Prerequisite.Negative`, `Prerequisite.Lists` & `[GenericPrerequisites]` section.
- If it finds a trigger with 5000 current probability weight then discard valid triggers all and start searching all valid triggers with weight 5000. AI will pick 1 randomly and decrease by 1 the current weight of the selected trigger (so if nothing happens in the next teams selection loop it won't appear in this special list). Under this scenario categories are disabled.
- Units can override the category using `ConsideredVehicle` and `ConsideredNaval` boolean tags.
- AI is be able to use unlocked units in captured Secret Labs.

- 新的 AI 系统，用于在多人游戏中随机选择有效的触发器。与原始方法不同，此方法检查先决条件并处理其他详细信息。
- 它可以将有效的触发器分为4个类别：地面、空中、海军和混合类别。如果设置，AI 将在随机类别中选取一个随机触发器。
- 类别可以具有不同的机会概率。它可以全局设置或按所属方自定义。默认情况下，每个类别都有 25% 的机会被选中。
- 'NewTeamsSelector.MergeUnclassifiedCategoryWith' 可用于将混合类别（单元来自不同类别）合并到主要类别之一。
- 如果选择没有有效触发器的类别，则存在允许从所有有效触发器中选取触发器的回退模式，就像禁用类别时一样。
- 如果 'Autocreate=yes'，AI会关心所有单位的先决条件，所以如果所属方的科技树对于触发器来说不完整，它就会被丢弃。它尊重像'Prerequisite.RequiredTheaters'、'Prerequisite.Negative'、'Prerequisite.Lists'和'[GenericPrerequisites]'部分这样的Ares标签。
- 如果找到当前概率权重为 5000 的触发器，则丢弃所有有效触发器，并开始搜索权重为 5000 的所有有效触发器。AI 将随机选择 1 个，并将所选触发器的当前权重减少 1（因此，如果在下一个团队选择循环中没有发生任何事情，它不会出现在此特殊列表中）。在此方案下，类别处于禁用状态。
- 单位可以使用 'ConsideredVehicle' 和 'ConsideredNaval' 布尔标签覆盖类别。
- AI 能够在被占领的秘密实验室中使用已解锁的单位。

In `rulesmd.ini`:
```ini
[AI]
NewTeamsSelector=false                               ; boolean
NewTeamsSelector.SplitTriggersByCategory=true        ; boolean
NewTeamsSelector.EnableFallback=false                ; boolean
NewTeamsSelector.GroundCategoryPercentage=0.25       ; floating point value, percents or absolute
NewTeamsSelector.NavalCategoryPercentage=0.25        ; floating point value, percents or absolute
NewTeamsSelector.AirCategoryPercentage=0.25          ; floating point value, percents or absolute
NewTeamsSelector.UnclassifiedCategoryPercentage=0.25 ; floating point value, percents or absolute
NewTeamsSelector.MergeUnclassifiedCategoryWith=-1    ; Integer - Ground: 1, Air: 2, Naval: 3

[SOMEHOUSE]                                       ; HouseType
NewTeamsSelector.MergeUnclassifiedCategoryWith=   ; boolean
NewTeamsSelector.UnclassifiedCategoryPercentage=  ; floating point value, percents or absolute
NewTeamsSelector.GroundCategoryPercentage=        ; floating point value, percents or absolute
NewTeamsSelector.AirCategoryPercentage=           ; floating point value, percents or absolute
NewTeamsSelector.NavalCategoryPercentage=         ; floating point value, percents or absolute

[SOMETECHNO]        ; TechnoType
ConsideredNaval=    ; boolean
ConsideredVehicle=  ; boolean
```


- 略微修改了 AI 触发条件 0（敌人拥有...）、1（所属方拥有...）和 7（平民拥有...），并添加了 10 个新条件（从 8 到 17）来检查对象列表，而不仅仅是 1 个单位。前 3 个被修改是因为对象计数器没有实时更新。
- 可能的修改情况和新情况是：

|   *条件*    |    *对象列表*     |                 *描述*                          |
| :---------: | :---------------: | :---------------------------------------------: |
| 0 (changed) | No                | 计算主要敌人的单位。如果AI所属方没有主要敌人，它将搜索所有敌人。在原版 YR 中，如果所属方没有主要敌人，则此条件失败 |
| 1 (changed) | No                | 计算自身的单位的数量 |
| 7 (changed) | No                | 计算平民的单位的数量 |
| 8           | No                | 计算敌方单位的数量。它会搜索所有敌人 |
| 9           | Yes               | 计算敌方单位的数量。如果AI所属方没有主要敌人，它将搜索所有敌人。 |
| 10          | Yes               | 计算自身的单位的数量 |
| 11          | Yes               | 计算平民的单位的数量 |
| 12          | Yes               | 计算敌方单位的数量。它会搜索所有敌人 |
| 13          | Yes               | 计算友方单位的数量 |
| 14          | Yes               | 计算敌方单位的数量。如果AI所属方没有主要敌人，它将搜索所有敌人。它使用 'AND' 比较器，因此每个对象都必须遵循相同的比较 |
| 15          | Yes               | 计算自身的单位的数量。它使用 'AND' 比较器，因此每个对象都必须遵循相同的比较 |
| 16          | Yes               | 计算敌方单位的数量。它使用 'AND' 比较器，因此每个对象都必须遵循相同的比较 |
| 17          | Yes               | 计算敌方单位的数量。它将搜索所有敌人。它使用 'AND' 比较器，因此每个对象都必须遵循相同的比较 |
| 18          | No                | 计算与被毁桥梁关联的 'BridgeRepairHut=yes' 的结构 |
| 19          | No                | 对 'BridgeRepairHut=yes' 与未损坏桥梁链接的结构进行计数 |

- Some trigger conditions need to specify a 0-based list from `[AITargetTypes]`. The index of the list is written in hexadecimal and in little endian format. The value must be written at the end of the trigger:
`00000000000000000000000000000000000000000000000000000000AABBCCDD`
For example: list 0 is written 00 (put it in AA), list 1 is written 01 (put it in AA), list 10 is written 0A (put it in AA), 255 is 0xFF (put it in AA) and 256 is 0x0001 (put it in AABB), etc.
- The *`AITargetTypes` index#* values are obtained in the new `AITargetTypes` section that must be declared in `rulesmd.ini`:

In `rulesmd.ini`:
```ini
[AITargetTypes]  ; List of TechnoType lists
0=SOMETECHNOTYPE,SOMEOTHERTECHNOTYPE,SAMPLETECHNOTYPE
1=ANOTHERTECHNOTYPE,YETANOTHERTECHNOTYPE
; ...
```
