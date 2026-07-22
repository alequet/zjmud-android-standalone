// Attribute-only retention policy used after redistributing innate attributes.

private mapping base_minimums = ([
	"beiming-shengong" : ([ "int" : 32, "con" : 24 ]),
	"bianfu-bu" : ([ "dex" : 26 ]),
	"chuixiao-jifa" : ([ "int" : 24 ]),
	"diyang-chufan" : ([ "str" : 30 ]),
	"dragon-strike" : ([ "str" : 31, "con" : 24 ]),
	"feilong-zaitian" : ([ "str" : 30 ]),
	"hongjian-yulu" : ([ "str" : 30 ]),
	"huoyue-zaiyuan" : ([ "str" : 30 ]),
	"jianlong-zaitian" : ([ "str" : 30 ]),
	"jiuyang-shengong" : ([ "int" : 25, "con" : 29 ]),
	"kanglong-youhui" : ([ "str" : 30 ]),
	"lishe-dachuan" : ([ "str" : 30 ]),
	"lonely-sword" : ([ "int" : 34 ]),
	"longxiang" : ([ "str" : 30 ]),
	"longzhan-yuye" : ([ "str" : 30 ]),
	"lvshuang-bingzhi" : ([ "str" : 30 ]),
	"miyun-buyu" : ([ "str" : 30 ]),
	"piaoxue-zhang" : ([ "str" : 24, "con" : 23 ]),
	"qianlong-wuyong" : ([ "str" : 30 ]),
	"shenlong-baiwei" : ([ "str" : 30 ]),
	"shicheng-liulong" : ([ "str" : 30 ]),
	"shuanglong-qushui" : ([ "str" : 30 ]),
	"six-finger" : ([ "int" : 32, "con" : 26 ]),
	"sunze-youfu" : ([ "str" : 30 ]),
	"taixuan-gong" : ([ "str" : 25, "int" : 19, "con" : 25, "dex" : 25 ]),
	"tanqin-jifa" : ([ "int" : 24 ]),
	"turu-qilai" : ([ "str" : 30 ]),
	"xixing-dafa" : ([ "con" : 20 ]),
	"yijin-duangu" : ([ "int" : 26 ]),
	"yuyue-yuyuan" : ([ "str" : 30 ]),
	"zhenjing-baili" : ([ "str" : 30 ]),
]);

private mapping effective_minimums = ([
	"dagou-bang" : ([ "int" : 30 ]),
	"feixing-shu" : ([ "dex" : 25 ]),
	"jiaohua-bangfa" : ([ "dex" : 26 ]),
	"jiuyin-baiguzhao" : ([ "str" : 30 ]),
	"juemen-gun" : ([ "dex" : 23 ]),
	"kuangfeng-blade" : ([ "dex" : 25 ]),
	"kuangfeng-jian" : ([ "dex" : 25 ]),
	"mantian-xing" : ([ "dex" : 25 ]),
	"mantianhuayu-zhen" : ([ "dex" : 25 ]),
	"ningxue-shenzhao" : ([ "str" : 40, "dex" : 40 ]),
	"piaoxue-zhang" : ([ "str" : 52, "con" : 50 ]),
	"rouyun-steps" : ([ "dex" : 31 ]),
	"sad-strike" : ([ "str" : 45 ]),
	"shenghuo-ling" : ([ "int" : 35, "dex" : 25 ]),
	"sougu" : ([ "str" : 33 ]),
	"surge-force" : ([ "str" : 45 ]),
	"xuantie-jian" : ([ "str" : 45 ]),
	"yufeng-zhen" : ([ "dex" : 26 ]),
]);

private int query_attribute(object me, string attribute, int effective)
{
	if (!effective)
		return me->query(attribute);

	switch (attribute)
	{
	case "str": return me->query_str();
	case "int": return me->query_int();
	case "con": return me->query_con();
	case "dex": return me->query_dex();
	}
	return 0;
}

private int meets_minimums(object me, mapping minimums, int effective)
{
	int i;
	string *attributes;

	if (!mapp(minimums))
		return 1;

	attributes = keys(minimums);
	for (i = 0; i < sizeof(attributes); i++)
		if (query_attribute(me, attributes[i], effective) < minimums[attributes[i]])
			return 0;
	return 1;
}

int valid_for_attributes(string skill, object me)
{
	int layer;

	if (!meets_minimums(me, base_minimums[skill], 0) ||
	    !meets_minimums(me, effective_minimums[skill], 1))
		return 0;

	switch (skill)
	{
	case "bianfu-bu":
		return me->query("str") + me->query("relife/zhuanshi") <= 25;
	case "bluesea-force":
		return me->query("con") >= 22 || me->query_con() >= 63;
	case "count":
		return me->query_skill("count", 1) <= me->query("int") * 10;
	case "guzheng-jifa":
		return me->query("int") >= 24 || me->query_int() >= 35;
	case "jiuyin-shengong":
		return me->query("str") >= 22 || me->query_str() >= 63;
	case "kuihua-mogong":
		return me->query("dex") >= 22 || me->query_dex() >= 63;
	case "never-defeated":
		return me->query("int") >= 22 || me->query_int() >= 52;
	case "qiankun-danuoyi":
		layer = me->query_skill("qiankun-danuoyi", 1) / 50;
		if (layer > 13) layer = 13;
		return me->query("int") >= 22 + layer;
	}

	return 1;
}
