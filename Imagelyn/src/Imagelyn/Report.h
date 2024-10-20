#pragma once
#include "imagelynpch.h"

#include "Location.h"
#include "Client.h"

namespace Imagelyn {

	class Report
	{

	private:

		/* Used to show which activities went well or went porely with which preferences */
		struct Pairing
		{
			std::shared_ptr<Activity> m_Activity;
			std::shared_ptr<Preference> m_Preference;
			bool m_IsPositive;

			Pairing(std::shared_ptr<Activity> activity, std::shared_ptr<Preference> preference, bool isPositive)
				: m_Activity(activity), m_Preference(preference), m_IsPositive(isPositive)
			{
				if (isPositive)
					activity->UnlockPositiveHint(preference);
				else
					activity->UnlockNegativeHint(preference);
			}
		};

	private:

		/* The score generated by this report */
		int m_Score;

		/* A list of all preference/activity pairing in this report */
		std::vector<Pairing> m_Pairings;

	public:

		Report(const Client& client, const std::vector<std::shared_ptr<Activity>>& activities);

		void PrintReport() const;

	};

	class ReportManager
	{

	private:

		/* A list of all reports that have been generated */
		static std::vector<std::shared_ptr<Report>> m_Reports;

	public:

		static const std::shared_ptr<Report>& GenerateReport(const Client& client, const std::vector<std::shared_ptr<Activity>>& activities);

	};

}

