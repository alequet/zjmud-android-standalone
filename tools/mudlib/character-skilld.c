// Character-only retention policy for learned skills.

private mapping required_character = ([
	"jiuyin-shengong" : "光明磊落",
	"bluesea-force" : "心狠手辣",
	"never-defeated" : "狡黠多变",
	"kuihua-mogong" : "阴险奸诈",
	"lonely-sword" : "狡黠多变",
]);

private mapping forbidden_characters = ([
	"huagong-dafa" : ({ "光明磊落", "狡黠多变" }),
	"poison" : ({ "光明磊落" }),
	"xixing-dafa" : ({ "光明磊落" }),
]);

int valid_for_character(string skill, string character)
{
	string required;
	string *forbidden;

	required = required_character[skill];
	if (stringp(required))
		return character == required;

	forbidden = forbidden_characters[skill];
	if (pointerp(forbidden))
		return member_array(character, forbidden) == -1;

	// Attribute, equipment and level requirements only govern further growth.
	return 1;
}

mapping query_policy()
{
	return ([
		"required" : copy(required_character),
		"forbidden" : copy(forbidden_characters),
	]);
}
