CREATE TABLE `das_maxbot_user` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `first_name` varchar(64) NOT NULL,
  `last_name` varchar(64) NOT NULL,
  `user_name` varchar(32) NOT NULL,
  `lang` varchar(16) NOT NULL,
  `private_chat_id` bigint(20) DEFAULT NULL,
  `user_id` int(11) DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `das_maxbot_user_user_id_c1a42fc9_fk_das_user_id` (`user_id`),
  CONSTRAINT `das_maxbot_user_user_id_c1a42fc9_fk_das_user_id` FOREIGN KEY (`user_id`) REFERENCES `das_user` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=2059188665 DEFAULT CHARSET=utf8mb3 COLLATE=utf8mb3_bin;

CREATE TABLE `das_maxbot_auth` (
  `external_user_id` int(11) NOT NULL,
  `expired` bigint(20) NOT NULL,
  `token` varchar(512) NOT NULL,
  PRIMARY KEY (`external_user_id`),
  CONSTRAINT `das_maxbot_auth_external_user_id_fb8689b6_fk_das_maxbot_user_id` FOREIGN KEY (`external_user_id`) REFERENCES `das_maxbot_user` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3 COLLATE=utf8mb3_bin;

CREATE TABLE `das_maxbot_chat` (
  `id` bigint(20) NOT NULL,
  `admin_id` int(11) DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `das_maxbot_chat_admin_id_b96acf59_fk_das_maxbot_user_id` (`admin_id`),
  CONSTRAINT `das_maxbot_chat_admin_id_b96acf59_fk_das_maxbot_user_id` FOREIGN KEY (`admin_id`) REFERENCES `das_maxbot_user` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3 COLLATE=utf8mb3_bin;

CREATE TABLE `das_maxbot_subscriber` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `chat_id` bigint(20) NOT NULL,
  `group_id` int(11) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `das_maxbot_subscriber_group_id_f7dc9d18_fk_das_scheme_group_id` (`group_id`),
  CONSTRAINT `das_maxbot_subscriber_group_id_f7dc9d18_fk_das_scheme_group_id` FOREIGN KEY (`group_id`) REFERENCES `das_scheme_group` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=47 DEFAULT CHARSET=utf8mb3 COLLATE=utf8mb3_bin;
