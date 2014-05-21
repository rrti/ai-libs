cfg = {
	["version"] = "0.123",

	["SpamSpamSpam"] = {
		["Weight"] = 100,

		["BuilderLists"] = {
			-- Arm units
			["armcom"] = {
				[1] = {
					["Weight"] = 100,
					["Items"] = {
						[1] = {
							buildeeDef      = "armmex",
							repeatCount     = 1,
						},
						[2] = {
							buildeeDef      = "armllt",
							repeatCount     = 1,
						},
						[3] = {
							buildeeDef      = "armsolar",
							repeatCount     = 1,
						},
						[4] = {
							buildeeDef      = "armmex",
							repeatCount     = 1,
						},
						[5] = {
							buildeeDef      = "armllt",
							repeatCount     = 1,
						},
						[6] = {
							buildeeDef      = "armsolar",
							repeatCount     = 1,
						},
						[7] = {
							buildeeDef      = "armmex",
							repeatCount     = 1,
						},
						[8] = {
							buildeeDef      = "armvp",
							repeatCount     = 1,
							unitLimit       = 4,
							buildLimit      = 2,
							forceStartBuild = 1,
							minFrameSpacing = 30 * 60 * 10,
						},
						[9] = {
							buildeeDef      = "armllt",
							repeatCount     = 1,
						},
					},
				},
			},

			["armcv"] = {
				[1] = {
					["Weight"] = 75,
					["Items"] = {
						[1] = {
							buildeeDef      = "armsolar",
							repeatCount     =      1,
							unitLimit       =   9999,        -- how many units of this type are allowed to exist in total
							buildLimit      =    100,        -- how many units of this type may be built in parallel

							minMetalIncome  =      0,
							maxMetalIncome  =   1000,
							minEnergyIncome =      0,
							maxEnergyIncome =   9009,

							minMetalLevel   =     50,
							maxMetalLevel   =  50000,
							minEnergyLevel  =      0,
							maxEnergyLevel  = 100000,

							minGameFrame    =      0,
							maxGameFrame    = 999999,

							forceStartBuild = 1,
						},

						[2] = {
							buildeeDef      = "armmex",
							repeatCount     = 2,
							forceStartBuild = 1,
						},
					},
				},

				[2] = {
					["Weight"] = 25,
					["Items"] = {
						[1] = {
							buildeeDef      = "armllt",
						},
					},
				},
			},

			["armvp"] = {
				[1] = {
					["Weight"] = 100,
					["Items"] = {
						[1] = {
							buildeeDef      = "armflash",
							repeatCount     = 1,
							forceStartBuild = 1,
						},
						[2] = {
							buildeeDef      = "armfav",
							repeatCount     = 1,
							forceStartBuild = 1,
						},
						[3] = {
							buildeeDef      = "armflash",
							repeatCount     = 1,
							forceStartBuild = 1,
						},
					--	[4] = {
					--		buildeeDef      = "armfav",
					--		repeatCount     = 1,
					--		forceStartBuild = 1,
					--	},
						[5] = {
							buildeeDef      = "armcv",
							repeatCount     = 1,
							forceStartBuild = 1,
						},
						[6] = {
							buildeeDef      = "armstump",
							repeatCount     = 1,
							forceStartBuild = 1,
							minGameFrame    = 30 * 60 * 6,
						},
					},
				},
			},


			-- Core units
			["corcom"] = {
				[1] = {
					["Weight"] = 100,
					["Items"] = {
						[1] = {
							buildeeDef      = "cormex",
							repeatCount     = 1,
						},
						[2] = {
							buildeeDef      = "corllt",
							repeatCount     = 1,
						},
						[3] = {
							buildeeDef      = "corsolar",
							repeatCount     = 1,
						},
						[4] = {
							buildeeDef      = "cormex",
							repeatCount     = 1,
						},
						[5] = {
							buildeeDef      = "corllt",
							repeatCount     = 1,
						},
						[6] = {
							buildeeDef      = "corsolar",
							repeatCount     = 1,
						},
						[7] = {
							buildeeDef      = "cormex",
							repeatCount     = 1,
						},
						[8] = {
							buildeeDef      = "corvp",
							repeatCount     = 1,
							unitLimit       = 4,
							buildLimit      = 2,
							forceStartBuild = 1,
							minFrameSpacing = 30 * 60 * 10,
						},
						[9] = {
							buildeeDef      = "corllt",
							repeatCount     = 1,
						},
					},
				},
			},

			["corcv"] = {
				[1] = {
					["Weight"] = 75,
					["Items"] = {
						[1] = {
							buildeeDef      = "corsolar",
							repeatCount     =      1,
							unitLimit       =   9999,
							buildLimit      =    100,

							minMetalIncome  =      0,
							maxMetalIncome  =   1000,
							minEnergyIncome =      0,
							maxEnergyIncome =   9009,

							minMetalLevel   =     50,
							maxMetalLevel   =  50000,
							minEnergyLevel  =      0,
							maxEnergyLevel  = 100000,

							minGameFrame    =      0,
							maxGameFrame    = 999999,

							forceStartBuild = 1,
						},

						[2] = {
							buildeeDef      = "cormex",
							repeatCount     = 2,
							forceStartBuild = 1,
						},
					},
				},

				[2] = {
					["Weight"] = 25,
					["Items"] = {
						[1] = {
							buildeeDef      = "corllt",
						},
					},
				},
			},

			["corvp"] = {
				[1] = {
					["Weight"] = 100,
					["Items"] = {
						[1] = {
							buildeeDef      = "corgator",
							repeatCount     = 1,
							forceStartBuild = 1,
						},
						[2] = {
							buildeeDef      = "corfav",
							repeatCount     = 1,
							forceStartBuild = 1,
						},
						[3] = {
							buildeeDef      = "corgator",
							repeatCount     = 1,
							forceStartBuild = 1,
						},
					--	[4] = {
					--		buildeeDef      = "corfav",
					--		repeatCount     = 1,
					--		forceStartBuild = 1,
					--	},
						[5] = {
							buildeeDef      = "corcv",
							repeatCount     = 1,
							forceStartBuild = 1,
						},
						[6] = {
							buildeeDef      = "corraid",
							repeatCount     = 2,
							forceStartBuild = 1,
							minGameFrame    = 30 * 60 * 6,
						},
					},
				},
			},
		},






		["AttackerLists"] = {
			-- Arm units
			["armflash"] = {
				[1] = {
					["Weight"] = 100,
					["Items"] = {
						[1] = {
							attackeeDef        = "corgator",
							minGroupSize       = 4,
						},
						[2] = {
							attackeeDef        = "corak",
							minGroupSize       = 1,
							maxGroupSize       = 4,
						},
						[3] = {
							attackeeDef        = "corcom",
							minGroupSize       = 20,
							maxGroupSize       = 9000,
						},
						[4] = {
							attackeeDef        = "cormex",
							minGroupSize       = 8,
						},
						[5] = {
							attackeeDef        = "corsolar",
							minGroupSize       = 10,
						},
						[6] = {
							attackeeDef        = "corlab",
							minGroupSize       = 15,
						},
					},
				},
			},

			["armfav"] = {
				-- Jeffy
				[1] = {
					["Weight"] = 100,
					["Items"] = {
						[2] = {
							attackeeDef        = "corfav",
						},
						[3] = {
							attackeeDef        = "cormex",
							minGroupSize       = 2,
							maxGroupSize       = 3,
						},
						[4] = {
							attackeeDef        = "corsolar",
							minGroupSize       = 3,
							maxGroupSize       = 5,
						},
					},
				},
			},

			["armstump"] = {
				[1] = {
					["Weight"] = 100,
					["Items"] = {
						[1] = {
							attackeeDef        = "armham",
							minGroupSize       = 4,
						},
						[2] = {
							attackeeDef        = "armrock",
							minGroupSize       = 4,
						},
						[3] = {
							attackeeDef        = "armwar",
							minGroupSize       = 4,
						},
					},
				},
			},


			-- Core units
			["corgator"] = {
				[1] = {
					["Weight"] = 100,
					["Items"] = {
						[1] = {
							attackeeDef        = "armflash",
							minGroupSize       = 1,
						},
						[2] = {
							attackeeDef        = "armpw",
							minGroupSize       = 1,
							maxGroupSize       = 4,
						},
						[3] = {
							attackeeDef        = "armcom",
							minGroupSize       = 20,
							maxGroupSize       = 9000,
						},
						[4] = {
							attackeeDef        = "armmex",
							minGroupSize       = 8,
						},
						[5] = {
							attackeeDef        = "armsolar",
							minGroupSize       = 10,
						},
						[6] = {
							attackeeDef        = "armlab",
							minGroupSize       = 15,
						},
					},
				},
			},

			["corfav"] = {
				-- Weasel
				[1] = {
					["Weight"] = 100,
					["Items"] = {
						[1] = {
							attackeeDef        = "armflea",
						},
						[2] = {
							attackeeDef        = "armfav",
						},
						[3] = {
							attackeeDef        = "armmex",
							minGroupSize       = 2,
							maxGroupSize       = 3,
						},
						[4] = {
							attackeeDef        = "armsolar",
							minGroupSize       = 3,
							maxGroupSize       = 5,
						},
					},
				},
			},

			["corraid"] = {
				[1] = {
					["Weight"] = 100,
					["Items"] = {
						[1] = {
							attackeeDef        = "armham",
							minGroupSize       = 4,
						},
						[2] = {
							attackeeDef        = "armrock",
							minGroupSize       = 4,
						},
						[3] = {
							attackeeDef        = "armwar",
							minGroupSize       = 4,
						},
					},
				},
			},
		},
	},
}
