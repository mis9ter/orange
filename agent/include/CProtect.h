#pragma once
/*
	프로세스 보호

	https://blog.naver.com/gloryo/110182565162

*/
class CProtect
{
public:
	CProtect() {
		DenyForDynamicCode();
		DenyForDllInjection();
	}
	~CProtect() {
	
	}


	bool	DenyForDynamicCode() {
		/*
			https://blog.naver.com/PostView.naver?blogId=gloryo&logNo=220580776989&parentCategoryNo=&categoryNo=&viewDate=&isShowPopularPosts=false&from=postView
		*/
		PROCESS_MITIGATION_DYNAMIC_CODE_POLICY	policy;
		ZeroMemory(&policy, sizeof(policy));

		policy.ProhibitDynamicCode	= TRUE;
		return SetProcessMitigationPolicy(ProcessDynamicCodePolicy, &policy, sizeof(policy))? true : false;
	}
	bool	DenyForDllInjection() {
		/*
			https://blog.naver.com/PostView.nhn?blogId=gloryo&logNo=220576807227
		*/
		PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY	policy;
		ZeroMemory(&policy, sizeof(policy));
		policy.MicrosoftSignedOnly	= TRUE;
		return SetProcessMitigationPolicy(ProcessSignaturePolicy, &policy, sizeof(policy))? true : false;
	}

private:

};
